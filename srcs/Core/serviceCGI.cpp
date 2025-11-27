#include "service.hpp"


	// std::cout << "### current buffer: " << cgi.get_output_buffer() << std::endl;

	// std::cout << "\n ==> fds['poll_fds'][i].fd: " << fds["poll_fds"][i].fd << std::endl;
	// std::cout << "\n ==> cgi.get_pipe_to_cgi(): " << cgi.get_pipe_to_cgi() << std::endl;
	// std::cout << "\n ==> cgi.get_pipe_from_cgi(): " << cgi.get_pipe_from_cgi() << std::endl;

	// std::cout << "\n ==> revents value: " << fds["poll_fds"][i].revents << std::endl;
  // std::cout << "\n ==> POLLIN flag: " << POLLIN << std::endl;
  // std::cout << "\n ==> POLLOUT flag: " << POLLOUT << std::endl;
  // std::cout << "\n ==> POLLHUP flag: " << POLLHUP << std::endl;
  // std::cout << "\n ==> revents & POLLIN: " << (fds["poll_fds"][i].revents & POLLIN) << std::endl;
  // std::cout << "\n ==> cgi.can_i_process_and_write(): " << cgi.can_i_process_and_write() << std::endl;

  // // Check each condition separately
  // bool fd_matches = (fds["poll_fds"][i].fd == cgi.get_pipe_from_cgi());
  // bool has_pollin = (fds["poll_fds"][i].revents & POLLIN);
  // bool can_process = cgi.can_i_process_and_write();

  // std::cout << "\n ==> Condition breakdown:" << std::endl;
  // std::cout << "     fd_matches: " << fd_matches << std::endl;
  // std::cout << "     has_pollin: " << has_pollin << std::endl;
  // std::cout << "     can_process: " << can_process << std::endl;

void Service::cgi_handler(int i)
{
	std::cout << "\n###################################################" << std::endl;
	std::cout << "################## CGI REQUEST #######################" << std::endl;
	std::cout << "###################################################\n"
			  << std::endl;

	CGIProcess &cgi = (*cgi_processes[cgi_fd_for_cgi(fds["poll_fds"][i].fd , fds["cgi_fds"])]);
	Client &client = clients[cgi.get_client_fd()];

	if (fds["poll_fds"][i].fd == cgi.get_pipe_to_cgi() &&
				(fds["poll_fds"][i].revents & (POLLOUT | POLLHUP)) && cgi.can_i_read())
	{
		std::cout << "\n ==> handle writing to CGI" << std::endl;
		std::string res = client._request._body.substr(cgi.get_bytes_written()).substr(0, 1024);
		std::cout << "\n ==> this is what I write to CGI: " << res << std::endl;
		ssize_t bytes_sent = write(cgi.get_pipe_to_cgi(), res.c_str(), res.size());
		if (bytes_sent == -1)
		{
			std::cout << "Error sending data to CGI" << std::endl;
			client.set_status_code(500);
			client.set_flags_error();
			remove_fd(cgi.get_pipe_to_cgi());
			remove_fd(cgi.get_pipe_from_cgi());
			delete &cgi;
			return;
		}
		else if (bytes_sent == 0)
		{
			remove_fd(cgi.get_pipe_to_cgi());
			cgi.set_processing_and_writing();
		}
		else
			cgi.update_bytes_written(bytes_sent);
	}
	else if (fds["poll_fds"][i].fd == cgi.get_pipe_from_cgi() &&
						(fds["poll_fds"][i].revents & (POLLIN | POLLHUP)) && cgi.can_i_process_and_write())
	{
		std::cout << "\n ==> handle reading from CGI" << std::endl;

		char buffer[1024];
		ssize_t n = read(cgi.get_pipe_from_cgi(), buffer, sizeof(buffer));

		std::cout << "n is: " << n << std::endl;
		if (n > 0)
		{
			std::cout << "this is what we read: " << buffer << std::endl;
			cgi.append_to_output(buffer, n);
			std::cout << "### buffer right after saving: " << cgi.get_output_buffer() << std::endl;

		}
		else if (n == 0)  // EOF
		{
			int status;
			pid_t result = waitpid(cgi.get_pid(), &status, 0);
			if (result > 0 && WIFEXITED(status))
			{
				int exit_code = WEXITSTATUS(status);
				std::cout << "CGI exit code: " << exit_code << std::endl;

				if (exit_code == 0)
				{
					// Success - set response
					std::string output = cgi.get_output_buffer();
					std::cout << "CGI output (" << output.size() << " bytes): " << output << std::endl;
					size_t blank_line = output.find("\n\n");
					if (blank_line != std::string::npos)
					{
							client._response.set_header(output.substr(0, blank_line));
							output = output.substr(blank_line + 2);
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
			std::cout << "client response saved: " << client._response.get_response_data(0) << std::endl;
			remove_fd(cgi.get_pipe_from_cgi());
			delete &cgi;
			client.set_create_response();
		}
		else  // n < 0 - Error
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
	std::cout << "\n###################################################"<< std::endl;
	std::cout << "################## ADDING CGI #######################"<< std::endl;
	std::cout << "###################################################\n"<< std::endl;

	int pipe_to_cgi[2];
	int pipe_from_cgi[2];

	std::cout << "==> we will run the srcipt here: " << clients[this->fds["poll_fds"][i].fd]._request._fullPathURI.substr(1) << std::endl;

	char path_buf[PATH_MAX];
	ssize_t count = readlink("/proc/self/exe", path_buf, PATH_MAX);
	if (count == -1) {
			perror("Error getting executable path");
			exit(1);
	}
	path_buf[count] = '\0';
	std::string dir_path = std::string(path_buf);
	dir_path = dir_path.substr(0, dir_path.find_last_of("/"));
	std::string exec_path = dir_path + clients[this->fds["poll_fds"][i].fd]._request._fullPathURI.substr(1);
	std::cout << "This is the PATH: " << exec_path << std::endl;
	std::string arg0 = "/usr/bin/python3";

	if(pipe(pipe_to_cgi) || pipe(pipe_from_cgi))
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
		// std::cout << "Child starting ... " << std::endl;
		close(pipe_to_cgi[1]);
		close(pipe_from_cgi[0]);
		if(dup2(pipe_to_cgi[0], STDIN_FILENO) == -1)
			exit(1); // TODO: handle error
		if(dup2(pipe_from_cgi[1], STDOUT_FILENO) == -1)
			exit(1); // TODO: handle error
		// dup2(pipe_from_cgi[1], STDERR_FILENO); // Use for testing
		close(pipe_to_cgi[0]);
		close(pipe_from_cgi[1]);

		// std::cout << "## EXECVE about to star ... " << std::endl;
		// std::cout << "## EXECVE about to start ... " << std::endl;
		char *argv[] = {strdup(arg0.c_str()), strdup(exec_path.c_str()), NULL };
		char *envp[] = {NULL};
		execve(argv[0], argv, envp);
		exit(1); // TODO: handle error
	}
	else
	{
		CGIProcess* cgi = new CGIProcess(this->fds["poll_fds"][i].fd, pid, pipe_to_cgi[1], pipe_from_cgi[0]);
		std::cout << "Parent starting ... " << std::endl;
		close(pipe_to_cgi[0]);
		close(pipe_from_cgi[1]);
		std::cout << "\n ==> pipe_to_cgi[1] " << pipe_to_cgi[1] << std::endl;
		std::cout << "\n ==> pipe_from_cgi[0] " << pipe_from_cgi[0] << std::endl;
		if (clients[this->fds["poll_fds"][i].fd]._request._body.empty())
			close(pipe_to_cgi[1]);
		else
		{
			std::cout << "Adding writing pipe to CGI " << std::endl;
			struct pollfd ptc;
			ptc.fd = pipe_to_cgi[1];
			ptc.events = POLLOUT;
			ptc.revents = 0;
			this->fds["cgi_fds"].push_back(ptc);
			this->fds["poll_fds"].push_back(ptc);
			this->cgi_processes.insert(std::pair<int, CGIProcess * >(pipe_to_cgi[1], cgi) );
		}

		struct pollfd pfc;
		pfc.fd = pipe_from_cgi[0];
		pfc.events = POLLIN;
		pfc.revents = 0;
		this->fds["cgi_fds"].push_back(pfc);
		this->fds["poll_fds"].push_back(pfc);
		this->cgi_processes.insert(std::pair<int, CGIProcess * >(pipe_from_cgi[0], cgi) );

		std::cout << "Parent - everything saved ... " << std::endl;
		if (clients[this->fds["poll_fds"][i].fd]._request._body.empty())
			(*this->cgi_processes[pipe_from_cgi[0]]).set_processing_and_writing();

	}
	return;
}
