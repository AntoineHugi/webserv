#include "service.hpp"
#include <cmath>

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
	env_strings.push_back("SERVER_PROTOCOL=" + clients[fd]._request._version);
	ss.str("");
	ss << (*clients[fd].get_server()).get_port();
	env_strings.push_back("SERVER_PORT=" + ss.str());
	env_strings.push_back("SCRIPT_NAME=" + clients[fd]._request._uri);
	env_strings.push_back("REQUEST_URI=" + clients[fd]._request._uri);
	env_strings.push_back("PATH_INFO=" + clients[fd]._request._uri);

	// Add all HTTP headers as HTTP_* environment variables
	for (std::map<std::string, std::string>::const_iterator it = clients[fd]._request._header_kv.begin();
	     it != clients[fd]._request._header_kv.end(); ++it)
	{
		std::string header_name = it->first;
		std::string header_value = it->second;

		// Skip Content-Type and Content-Length (already added above)
		if (header_name == "Content-Type" || header_name == "Content-Length")
			continue;

		// Convert header name to CGI format: "X-Secret-Header" -> "HTTP_X_SECRET_HEADER"
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

	// if (clients[fd]._request._fullPathURI == "./YoupiBanane/youpi.bla" && clients[fd]._request._method == "POST")
	// {
	// 	std::cout << "=== SPECIAL CASE! Setting up CGI ENV for test script ===" << std::endl;
	// 	// env_strings.push_back("SCRIPT_NAME=" + clients[fd]._request._uri);
	// 	// env_strings.push_back("HTTP_ACCEPT=");
	// 	// env_strings.push_back("HTTP_ACCEPT_LANGUAGE=");
	// 	// env_strings.push_back("HTTP_COOKIE=");
	// 	// env_strings.push_back("HTTP_REFERER=");
	// 	// env_strings.push_back("HTTP_USER_AGENT=Go-http-client/1.1");
	// 	env_strings.push_back("PATH_INFO=/directory/youpi.bla");
	// 	// env_strings.push_back("PATH_TRANSLATED=/");
	// 	// env_strings.push_back("REDIRECT_STATUS=200");
	// 	// env_strings.push_back("REMOTE_ADDR=");
	// 	env_strings.push_back("SERVER_NAME=127.0.0.1");
	// 	env_strings.push_back("SERVER_SOFTWARE=webserv/1.0");
	// 	env_strings.push_back("REQUEST_URI=/directory/youpi.bla");
	// 	env_strings.push_back("SCRIPT_NAME=/directory/youpi.bla");
	// }
	// else
	// {
	// 	env_strings.push_back("CONTENT_LENGTH=" + ss.str());
	// 	env_strings.push_back("CONTENT_TYPE=" + clients[fd]._request._header_kv["Content-Type"]);
	// 	env_strings.push_back("QUERY_STRING=");
	// 	env_strings.push_back("SCRIPT_NAME=" + clients[fd]._request._fullPathURI);
	// 	env_strings.push_back("PATH_INFO=" + exec_path);
	// }

	char **envp = new char *[env_strings.size() + 1];
	for (size_t i = 0; i < env_strings.size(); i++)
		envp[i] = strdup(env_strings[i].c_str());
	envp[env_strings.size()] = NULL;

	if (DEBUG)
	{
		std::cout << "=== CGI Environment Variables ===" << std::endl;
		for (size_t i = 0; i < env_strings.size(); i++)
			std::cout << envp[i] << std::endl;
		std::cout << "=================================" << std::endl;
	}

	return envp;
}

void Service::cgi_handler(int i)
{
	if (DEBUG)
	{
		std::cout << "\n###################################################" << std::endl;
		std::cout << "################## CGI REQUEST #######################" << std::endl;
		std::cout << "###################################################\n"
			  << std::endl;
	}
	CGIProcess &cgi = (*cgi_processes[cgi_fd_for_cgi(fds["poll_fds"][i].fd, fds["cgi_fds"])]);
	Client &client = clients[cgi.get_client_fd()];

	// std::cout << "==> fds['poll_fds'][i].fd: " << fds["poll_fds"][i].fd << std::endl;
	// std::cout << "==> cgi.get_pipe_to_cgi(): " << cgi.get_pipe_to_cgi() << std::endl;
	// std::cout << "==> cgi.get_pipe_from_cgi(): " << cgi.get_pipe_from_cgi() << std::endl;
	// std::cout << "==> cgi.can_i_read(): " << cgi.can_i_read() << std::endl;
	// std::cout << "==> cgi.can_i_process_and_write(): " << cgi.can_i_process_and_write() << std::endl;
	// std::cout << "==> fds['poll_fds'][i].revents & (POLLOUT | POLLHUP): " << (fds["poll_fds"][i].revents & (POLLOUT | POLLHUP)) << std::endl;
	// std::cout << "==> fds['poll_fds'][i].revents & (POLLIN | POLLHUP): " << (fds["poll_fds"][i].revents & (POLLIN | POLLHUP)) << std::endl;
	// std::cout << "==> fds['poll_fds'][i].revents : " << fds["poll_fds"][i].revents << std::endl;
	// sleep(1);

	if (fds["poll_fds"][i].fd == cgi.get_pipe_to_cgi() && (fds["poll_fds"][i].revents & (POLLOUT | POLLHUP)))
	{
		// std::cout << "\n ==> handle writing to CGI" << std::endl;
		// std::cout << "\n ==> content length = " << client._request._body.size() << std::endl;
		//std::string res = client._request._body.substr(cgi.get_bytes_written()).substr(0, BUFFER_SIZE);
		std::string res = client._request._body.substr(cgi.get_bytes_written(), BUFFER_SIZE);
		//	std::cout << "res : " << res << std::endl;

		ssize_t bytes_sent = write(cgi.get_pipe_to_cgi(), res.c_str(), res.size());
		// std::cout << "\n ==> bytes_sent = " << bytes_sent << std::endl;
		if (bytes_sent == -1)
		{
			std::cout << "Error sending data to CGI" << std::endl;
			std::cout << "errno : " << strerror(errno) << std::endl;
			int status;
			waitpid(cgi.get_pid(), &status, 0);
			int exit_code = WEXITSTATUS(status);
			std::cout << "CGI exit code: " << exit_code << std::endl;
			client.set_status_code(500);
			client.set_flags_error();
			client.set_create_response();
			remove_fd(cgi.get_pipe_to_cgi());
			remove_fd(cgi.get_pipe_from_cgi());
			delete &cgi;
			return;
		}
		else if (bytes_sent == 0)
		{
			// sleep(1);
			if (DEBUG)
				std::cout << "End of sending things to CGI : " << bytes_sent << std::endl;
			remove_fd(cgi.get_pipe_to_cgi());
			cgi.set_processing_and_writing();
		}
		else
		{
			// std::cout << "wrte to CGI : " << bytes_sent << std::endl;
			// std::cout << "============>   Total bytes written: " << cgi.get_bytes_written() << " out of " << client._request._body.size() << std::endl;
			cgi.update_bytes_written(bytes_sent);
		}
	}
	else if (fds["poll_fds"][i].fd == cgi.get_pipe_from_cgi() && (fds["poll_fds"][i].revents & (POLLIN | POLLHUP)))
	{
		// std::cout << "\n ==> handle reading from CGI" << std::endl;

		char buffer[BUFFER_SIZE];
		ssize_t n = read(cgi.get_pipe_from_cgi(), buffer, sizeof(buffer));

		// std::cout << "n is: " << n << std::endl;
		if (n > 0)
		{
			// std::cout << "this is what we read: " << buffer << std::endl;
			cgi.append_to_output(buffer, n);
			// std::cout << "====>   Total buffer read: " << cgi.get_output_buffer().size() << " out of " << client._request._body.size() << std::endl;
		}
		else if (n == 0) // EOF
		{
			int status;
			pid_t result = waitpid(cgi.get_pid(), &status, 0);
			if (result > 0 && WIFEXITED(status))
			{
				int exit_code = WEXITSTATUS(status);
				if (DEBUG)
					std::cout << "CGI exit code: " << exit_code << std::endl;

				if (exit_code == 0)
				{
					// Success - set response
					std::string output = cgi.get_output_buffer();
					if (DEBUG)
					{
						std::cout << "CGI Buffer init: " << output.substr(0, 2048) << std::endl;
						std::cout << "CGI output (" << output.size() << " bytes): " << std::endl;
					}

					// Try to find blank line separator (CGI uses \r\n\r\n or \n\n)
					size_t blank_line = output.find("\r\n\r\n");
					size_t header_end_offset = 4; // Length of \r\n\r\n

					if (blank_line == std::string::npos)
					{
						// Fallback to \n\n for scripts that don't use \r\n
						blank_line = output.find("\n\n");
						header_end_offset = 2; // Length of \n\n
					}

					if (blank_line != std::string::npos)
					{
						client._response.set_header(output.substr(0, blank_line));
						output = output.substr(blank_line + header_end_offset);
					}
					client._response.set_body(output);
				}
				else
				{
					std::cerr << "CGI failed with exit code " << exit_code << std::endl;
					client.set_status_code(500);
				}
			}
			// client._response.set_body(cgi.get_output_buffer());
			// std::cout << "client response saved: " << client._response.get_response_data(0) << std::endl;
			remove_fd(cgi.get_pipe_from_cgi());
			delete &cgi;
			client.set_create_response();
		}
		else // n < 0 - Error
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				std::cerr << "Read error from CGI: " << strerror(errno) << std::endl;
				kill(cgi.get_pid(), SIGKILL);
				waitpid(cgi.get_pid(), NULL, 0);
				remove_fd(cgi.get_pipe_from_cgi());
				delete &cgi;
				client.set_status_code(500);
				client.set_create_response();
			}
		}
	}
	return;
}

void Service::setup_cgi_request(int i)
{
	if (DEBUG)
	{
		std::cout << "\n###################################################" << std::endl;
		std::cout << "################## ADDING CGI #######################" << std::endl;
		std::cout << "###################################################\n"
			  << std::endl;
	}

	int pipe_to_cgi[2];
	int pipe_from_cgi[2];

	if (DEBUG)
		std::cout << "==> we will run the srcipt here: " << clients[this->fds["poll_fds"][i].fd]._request._fullPathURI.substr(1) << std::endl;

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
	if (DEBUG)
		std::cout << "This is the PATH: " << exec_path << std::endl;
	std::string arg0 = clients[this->fds["poll_fds"][i].fd]._request._cgi_path;
	if (DEBUG)
		std::cout << "This is the executor: " << arg0 << std::endl;

	if (pipe(pipe_to_cgi) || pipe(pipe_from_cgi))
	{
		close(pipe_to_cgi[0]);
		close(pipe_to_cgi[1]);
		close(pipe_from_cgi[0]);
		close(pipe_from_cgi[1]);
		clients[this->fds["poll_fds"][i].fd].set_status_code(500);
		clients[this->fds["poll_fds"][i].fd].set_create_response();
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
		clients[this->fds["poll_fds"][i].fd].set_create_response();
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

		if (DEBUG)
			std::cerr << "execve failed: " << strerror(errno) << std::endl;
		exit(1);
	}
	else
	{
		CGIProcess *cgi = new CGIProcess(this->fds["poll_fds"][i].fd, pid, pipe_to_cgi[1], pipe_from_cgi[0]);
		if (DEBUG)
			std::cout << "Parent starting ... " << std::endl;
		close(pipe_to_cgi[0]);
		close(pipe_from_cgi[1]);
		fcntl(pipe_to_cgi[1], F_SETFL, fcntl(pipe_to_cgi[1], F_GETFL, 0) | O_NONBLOCK);
		fcntl(pipe_from_cgi[0], F_SETFL, fcntl(pipe_from_cgi[0], F_GETFL, 0) | O_NONBLOCK);
		if (DEBUG)
		{
			std::cout << "\n ==> pipe_to_cgi[1] " << pipe_to_cgi[1] << std::endl;
			std::cout << "\n ==> pipe_from_cgi[0] " << pipe_from_cgi[0] << std::endl;
		}
		if (clients[this->fds["poll_fds"][i].fd]._request._body.empty())
			close(pipe_to_cgi[1]);
		else
		{
			if (DEBUG)
				std::cout << "Adding writing pipe to CGI " << std::endl;
			add_poll_to_vectors(pipe_to_cgi[1], POLLOUT, "cgi_fds");
			this->cgi_processes.insert(std::pair<int, CGIProcess *>(pipe_to_cgi[1], cgi));
		}
		add_poll_to_vectors(pipe_from_cgi[0], POLLIN, "cgi_fds");
		this->cgi_processes.insert(std::pair<int, CGIProcess *>(pipe_from_cgi[0], cgi));

		if (DEBUG)
			std::cout << "Parent - everything saved ... " << std::endl;
		if (clients[this->fds["poll_fds"][i].fd]._request._body.empty())
			(*this->cgi_processes[pipe_from_cgi[0]]).set_processing_and_writing();
	}
	return;
}
