#include "service.hpp"
#include <cmath>

void free_env(char **envp)
{
	if (envp == NULL)
		return;
	for (int i = 0; envp[i] != NULL; i++)
		free(envp[i]);
	delete[] envp;
}

void Service::cleanup_cgi_process(int cgi_fd)
{
	if (cgi_processes.find(cgi_fd) == cgi_processes.end())
		return;

	CGIProcess *cgi = cgi_processes[cgi_fd];
	cgi_processes.erase(cgi_fd);

	// Check if any OTHER fd still references this pointer
	// (both pipe_to_cgi and pipe_from_cgi can point to same CGIProcess)
	bool is_still_referenced = false;
	for (std::map<int, CGIProcess *>::iterator it = cgi_processes.begin();
		 it != cgi_processes.end(); ++it)
	{
		if (it->second == cgi)
		{
			is_still_referenced = true;
			break;
		}
	}

	// Only delete if no other fd references this pointer
	if (!is_still_referenced && cgi != NULL)
		delete cgi;
}

char **setup_env(std::map<int, Client> clients, int fd)
{
	std::ostringstream ss;
	std::vector<std::string> env_strings;
	ss << clients[fd]._request._body.size();
	env_strings.push_back("CONTENT_LENGTH=" + ss.str());
	env_strings.push_back("CONTENT_TYPE=" + clients[fd]._request._header_kv["content-type"]);
	env_strings.push_back("QUERY_STRING=");
	env_strings.push_back("REQUEST_METHOD=" + clients[fd]._request._method);
	env_strings.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_strings.push_back("SERVER_PROTOCOL=" + clients[fd]._request.get_version());
	ss.str("");
	ss << (*clients[fd].get_server()).get_port();
	env_strings.push_back("SERVER_PORT=" + ss.str());
	env_strings.push_back("SCRIPT_NAME=" + clients[fd]._request._uri);
	env_strings.push_back("REQUEST_URI=" + clients[fd]._request._uri);
	env_strings.push_back("PATH_INFO=" + clients[fd]._request._uri);

	/* Add all HTTP headers as HTTP_* environment variables */
	for (std::map<std::string, std::string>::const_iterator it = clients[fd]._request._header_kv.begin();
		 it != clients[fd]._request._header_kv.end(); ++it)
	{
		std::string header_name = it->first;
		std::string header_value = it->second;

		/* Skip Content-Type and Content-Length (already added above) */
		if (header_name == "Content-Type" || header_name == "Content-Length")
			continue;

		/* Convert header name to CGI format: "X-Secret-Header" -> "HTTP_X_SECRET_HEADER" */
		std::string env_name = "HTTP_";
		for (size_t i = 0; i < header_name.length(); i++)
		{
			char c = header_name[i];
			if (c == '-')
				env_name += '_';
			else if (c >= 'a' && c <= 'z')
				env_name += std::toupper(c);
			else if (c >= 'A' && c <= 'Z')
				env_name += c;
			else
				env_name += c;
		}
		env_strings.push_back(env_name + "=" + header_value);
	}

	char **envp = new char *[env_strings.size() + 1];
	print_cyan("/n === CGI Environment Variables set up ===", DEBUG);

	for (size_t i = 0; i < env_strings.size(); i++)
	{
		envp[i] = strdup(env_strings[i].c_str());
		print_cyan(envp[i], DEBUG);
	}
	print_cyan(" =================================", DEBUG);

	envp[env_strings.size()] = NULL;
	return envp;
}

void Service::cgi_handler(int i)
{
	CGIProcess &cgi = (*cgi_processes[cgi_fd_for_cgi(fds["poll_fds"][i].fd, fds["cgi_fds"])]);
	Client &client = clients[cgi.get_client_fd()];
	client.update_last_interaction();

	if (fds["poll_fds"][i].fd == cgi.get_pipe_to_cgi() && (fds["poll_fds"][i].revents & (POLLOUT | POLLHUP)))
	{
		cgi.update_last_interaction();
		print_header("CGI REQUEST - Writing to CGI");
		std::string res = client._request._body.substr(cgi.get_bytes_written(), BUFFER_SIZE);
		ssize_t bytes_sent = write(cgi.get_pipe_to_cgi(), res.c_str(), res.size());
		if (bytes_sent == -1)
		{
			print_red("Error: Failed to send data to CGI", true);
			print_red("errno : " + convert_to_string(errno), true);
			int status;
			waitpid(cgi.get_pid(), &status, 0);
			int exit_code = WEXITSTATUS(status);
			print_red("CGI exited with code: " + convert_to_string(exit_code), true);

		client.set_status_code(500);
		client.set_flags_error();
		client.set_process_request();
		remove_fd(cgi.get_pipe_to_cgi());
		remove_fd(cgi.get_pipe_from_cgi());
		cleanup_cgi_process(cgi.get_pipe_to_cgi());
		cleanup_cgi_process(cgi.get_pipe_from_cgi());
		return;
		}
		else if (bytes_sent == 0)
		{
			print_blue("End of sending things to CGI : " + convert_to_string(bytes_sent), DEBUG);
			remove_fd(cgi.get_pipe_to_cgi());
			cgi.set_processing_and_writing();
		}
		else
		{
			print_cyan("Writing to CGI sucessful : " + convert_to_string(bytes_sent), DEBUG);
			cgi.update_bytes_written(bytes_sent);
		}
	}
	else if (fds["poll_fds"][i].fd == cgi.get_pipe_from_cgi() && (fds["poll_fds"][i].revents & (POLLIN | POLLHUP)))
	{
		cgi.update_last_interaction();
		print_header("CGI REQUEST - Reading from CGI");
		char read_buffer[1048576];
		ssize_t n = read(cgi.get_pipe_from_cgi(), read_buffer, BUFFER_SIZE);
		if (n > 0)
		{
			client._response.get_body().append(read_buffer, n);
			if (client._response.get_header().empty())
			{
				size_t blank_line = client._response.get_body().find("\r\n\r\n");
				size_t header_end_offset = 4;
				if(blank_line == std::string::npos)
				{
					blank_line = client._response.get_body().find("\n\n");
					header_end_offset = 2;
					if(blank_line == std::string::npos)
					{
						blank_line = 0;
						header_end_offset = 0;
					}
				}

				if (header_end_offset != 0)
				{
					client._response.set_header(client._response.get_body().substr(0, blank_line));
					client._response.get_body().erase(0, blank_line + header_end_offset);
				}

			}
			else
				client._response.set_body(client._response.get_body().append(client._response.get_response_buffer()));
			print_cyan("Reading from CGI sucessful : " + convert_to_string(n), DEBUG);
		}
		else if (n == 0)
		{
			int status;
			pid_t result = waitpid(cgi.get_pid(), &status, 0);
			if (result > 0 && WIFEXITED(status))
			{
				int exit_code = WEXITSTATUS(status);
				print_blue("CGI exit code: " + convert_to_string(exit_code), DEBUG);
				if (exit_code == 0)
				{
					print_yellow(client._response.get_header(), DEBUG);
				//	print_cyan("CGI output (" + convert_to_string(output.size()) + " bytes)", DEBUG);
				}
				else
				{
					print_red("CGI failed", true);
					std::cerr << "CGI failed with exit code " << exit_code << std::endl;
					client.set_status_code(500);
				}
		}
		int pipe_fd = cgi.get_pipe_from_cgi();
		remove_fd(pipe_fd);
		cleanup_cgi_process(pipe_fd);
		client.set_create_response();
	}
	else
	{
		std::cerr << "Read error from CGI: " << strerror(errno) << std::endl;
		kill(cgi.get_pid(), SIGKILL);
		waitpid(cgi.get_pid(), NULL, 0);
		int pipe_fd = cgi.get_pipe_from_cgi();
		remove_fd(pipe_fd);
		cleanup_cgi_process(pipe_fd);
		client.set_status_code(500);
		client.set_process_request();
	}
	}
	return;
}

void Service::setup_cgi_request(int i)
{
	print_header("ADDING CGI");
	int pipe_to_cgi[2];
	int pipe_from_cgi[2];

	char path_buf[PATH_MAX];
	ssize_t count = readlink("/proc/self/exe", path_buf, PATH_MAX);
	if (count == -1)
	{
		perror("Error getting executable path");
		exit(1);
	}
	path_buf[count] = '\0';
	std::string dir_path = std::string(path_buf);
	dir_path = dir_path.substr(0, dir_path.find_last_of("/"));
	std::string exec_path = dir_path + clients[this->fds["poll_fds"][i].fd]._request._fullPathURI.substr(1);
	std::string arg0 = clients[this->fds["poll_fds"][i].fd]._request._cgi_path;
	print_cyan("This is the file being executed: " + clients[this->fds["poll_fds"][i].fd]._request._fullPathURI.substr(1), DEBUG);
	print_cyan("This is the executor: " + arg0, DEBUG);

	if (pipe(pipe_to_cgi) || pipe(pipe_from_cgi))
	{
		close(pipe_to_cgi[0]);
		close(pipe_to_cgi[1]);
		close(pipe_from_cgi[0]);
		close(pipe_from_cgi[1]);
		clients[this->fds["poll_fds"][i].fd].set_status_code(500);
		clients[this->fds["poll_fds"][i].fd].set_process_request();
		return;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		close(pipe_to_cgi[0]);
		close(pipe_to_cgi[1]);
		close(pipe_from_cgi[0]);
		close(pipe_from_cgi[1]);
		clients[this->fds["poll_fds"][i].fd].set_status_code(500);
		clients[this->fds["poll_fds"][i].fd].set_process_request();
		return;
	}
	else if (pid == 0)
	{
		close(pipe_to_cgi[1]);
		close(pipe_from_cgi[0]);
		char *argv[] = {strdup(arg0.c_str()), strdup(exec_path.c_str()), NULL};
		char **envp = setup_env(this->clients, this->fds["poll_fds"][i].fd);

		if (dup2(pipe_to_cgi[0], STDIN_FILENO) == -1)
			exit(1);
		if (dup2(pipe_from_cgi[1], STDOUT_FILENO) == -1)
			exit(1);

		close(pipe_to_cgi[0]);
		close(pipe_from_cgi[1]);

		execve(argv[0], argv, envp);

		// If execve fails, clean up before exit
		if (DEBUG)
			std::cerr << "execve failed: " << strerror(errno) << std::endl;
		free_env(envp);
		free(argv[0]);
		free(argv[1]);
		exit(1);
	}
	else
	{
		CGIProcess *cgi = new CGIProcess(this->fds["poll_fds"][i].fd, pid, pipe_to_cgi[1], pipe_from_cgi[0]);
		print_blue("\nParent starting ... ", DEBUG);
		close(pipe_to_cgi[0]);
		close(pipe_from_cgi[1]);
		fcntl(pipe_to_cgi[1], F_SETFL, fcntl(pipe_to_cgi[1], F_GETFL, 0) | O_NONBLOCK);
		fcntl(pipe_from_cgi[0], F_SETFL, fcntl(pipe_from_cgi[0], F_GETFL, 0) | O_NONBLOCK);
		print_cyan("\n ==> pipe_to_cgi[1] " + convert_to_string(pipe_to_cgi[1]), DEBUG);
		print_cyan("==> pipe_from_cgi[0] \n" + convert_to_string(pipe_from_cgi[0]), DEBUG);
		if (clients[this->fds["poll_fds"][i].fd]._request._body.empty())
			close(pipe_to_cgi[1]);
		else
		{
			print_blue("Parent - adding writing pipe to CGI ", DEBUG);
			add_poll_to_vectors(pipe_to_cgi[1], POLLOUT, "cgi_fds");
			this->cgi_processes.insert(std::pair<int, CGIProcess *>(pipe_to_cgi[1], cgi));
		}
		add_poll_to_vectors(pipe_from_cgi[0], POLLIN, "cgi_fds");
		this->cgi_processes.insert(std::pair<int, CGIProcess *>(pipe_from_cgi[0], cgi));
		print_blue("Parent - everything saved ... ", DEBUG);
		if (clients[this->fds["poll_fds"][i].fd]._request._body.empty())
			(*this->cgi_processes[pipe_from_cgi[0]]).set_processing_and_writing();
	}
	return;
}
