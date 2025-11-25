#include "service.hpp"

void Service::cgi_handler(int i)
{
	std::cout << "HANDLING cgi HERE " << std::endl;
	CGIProcess &cgi = cgi_processes[cgi_fd_for_cgi(fds["poll_fds"][i].fd , fds["cgi_fds"])];
	Client &client = clients[fds["poll_fds"][i].fd];

	if (fds["poll_fds"][i].fd == cgi.get_pipe_to_cgi())
	{
		/* handle writing to CGI */
		std::string res = client._request._body.substr(cgi.get_bytes_written()).substr(0, 1024);
		ssize_t bytes_sent = send(cgi.get_pipe_to_cgi(), res.c_str(), res.size(), 0);
		if (bytes_sent == -1)
		{
			client.set_status_code(500);
			client.set_flags_error();
			std::cout << "Error sending data to CGI" << std::endl;
			// set status to finish cgi
			// set status to create response in the clinet
			return;
		}
		else if (bytes_sent == 0)
		{
			close(cgi.get_pipe_to_cgi());
			cgi_processes.erase(fds["poll_fds"][i].fd);
			// poll_fds.erase(poll_fds.begin() + i); // Remove from poll
			cgi.set_processing();
		}
		else
			cgi.update_bytes_written(bytes_sent);
	}
	else if (fds["poll_fds"][i].fd == cgi.get_pipe_from_cgi())
	{
		/* handle reading from CGI */

		char buffer[1024];
		ssize_t n = read(cgi.get_pipe_from_cgi(), buffer, sizeof(buffer));

		if (n > 0)
			cgi.append_to_output(buffer, n);
		else if (n == 0)  // EOF
		{
			// Child finished!

			// REAP CHILD HERE
			int status;
			waitpid(cgi.get_pid(), &status, 0);

		// 	// Parse CGI output and send to client
		// 	Client& client = clients[cgi._client_fd];
		// 	parse_cgi_output(cgi._output_buffer, client);
		// 	client.set_send_response();
		// // cgi.set_finish();
			// Cleanup
			close(cgi.get_pipe_from_cgi());
			delete &cgi;
			fds["cgi_fds"].erase(fds["poll_fds"][i].fd);
			fds["poll_fds"].erase(this->fds["poll_fds"].begin() + i);
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

	if(pipe(pipe_to_cgi) || pipe(pipe_from_cgi))
	{
		// TODO: handle error;
		return;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		return; // TODO: handle error
	}
	else if (pid == 0)
	{
		// Child process
		// Close unused ends of the pipes
		close(pipe_to_cgi[1]);
		close(pipe_from_cgi[0]);

		// Redirect stdin
		if(dup2(pipe_to_cgi[0], STDIN_FILENO) == -1)
			exit(1); // TODO: handle error
		// Redirect stdout
		if(dup2(pipe_from_cgi[1], STDOUT_FILENO) == -1)
			exit(1); // TODO: handle error
		close(pipe_to_cgi[0]);
		close(pipe_from_cgi[1]);
		// Execute CGI program
		// For demonstration, we'll use /usr/bin/env to simulate a CGI script
		//  char *argv[] = {"/usr/bin/python3", "/path/to/script.py", NULL};
		char *argv[] = {"/usr/bin/env", NULL};
		char *envp[] = {NULL};
		execve("/usr/bin/env", argv, envp);
		exit(1); // TODO: handle error

	}
	else
	{
		// Parent process
		// Close unused ends of the pipes
		close(pipe_to_cgi[0]);
		close(pipe_from_cgi[1]);

		if (clients[this->fds["poll_fds"][i].fd]._request._body.empty())
			close(pipe_to_cgi[1]);
		else
		{
			struct pollfd ptc;
			ptc.fd = pipe_to_cgi[1];
			ptc.events = POLLOUT;
			ptc.revents = 0;
			this->fds["cgi_fds"].push_back(ptc);
			this->fds["poll_fds"].push_back(ptc);
		}

		struct pollfd pfc;
		pfc.fd = pipe_from_cgi[0];
		pfc.events = POLLIN;
		pfc.revents = 0;
		this->fds["cgi_fds"].push_back(pfc);
		this->fds["poll_fds"].push_back(pfc);
		this->cgi_processes.insert(std::pair<int, CGIProcess>(pipe_from_cgi[0], CGIProcess(pipe_from_cgi[0], pid, pipe_to_cgi[1], pipe_from_cgi[0])));
	}
	return;
}

/*
CGIProcess Service::find_cgi_for_this_client(int i)
{
	std::map<int, CGIProcess>::iterator it = cgi_processes.begin();
	for (; it != cgi_processes.end(); ++it)
	{
		if (it->second.can_i_process() && it->second.get_client_fd() == this->fds["poll_fds"][i].fd)
		{
			// Found the CGI process for this client
			// Read output from CGI
			char buffer[1024];
			ssize_t bytes_read = read(it->second._pipe_from_cgi, buffer, sizeof(buffer) - 1);
			if (bytes_read > 0)
			{
				buffer[bytes_read] = '\0';
				clients[client_fd].response_append_body(std::string(buffer));
			}
			else if (bytes_read == 0)
			{
				// CGI finished
				it->second.set_writing();
			}
			else
			{
				// Handle read error
			}
			return;
		}
	}
}
*/
	// waitpid(pid, &status, WNOHANG);

	// This is for the CGI fd triggered in poll
	/*

	  Currently just prints a message. This should:
  - Identify which CGI process triggered the event
  - If POLLIN on pipe_from_cgi: read CGI output
  - If POLLOUT on pipe_to_cgi: write request body to CGI
  - If POLLHUP: CGI finished, call waitpid(), parse output, send response to client

	                **SHOULD DO:**
                • Find CGIProcess by fd
                • write() client._request._body to pipe_to_cgi[1]
                • If all written:
                    - close(pipe_to_cgi[1])
                    - Remove from poll_fds
                    - Mark cgi.finished_writing = true

	CGIProcess& cgi = cgi_processes[fd];  // or find it

	if (revents & POLLOUT && fd == cgi.pipe_to_cgi)
	{
			// Write request body to CGI stdin
			// ...
			if (finished_writing)
			{
					close(cgi.pipe_to_cgi);
					cgi.finished_writing = true;
					remove_from_poll(cgi.pipe_to_cgi);
			}
	}

	if (revents & POLLIN && fd == cgi.pipe_from_cgi)
	{
			// Read CGI output
			char buffer[4096];
			ssize_t n = read(fd, buffer, sizeof(buffer));
			if (n > 0)
					cgi.output_buffer.append(buffer, n);
			else if (n == 0)
			{
					// EOF - CGI finished
					close(cgi.pipe_from_cgi);
					int status;
					waitpid(cgi.pid, &status, 0);  // Reap child

					// Parse cgi.output_buffer and create HTTP response
					Client& client = clients[cgi.client_fd];
					parse_cgi_output_to_response(client, cgi.output_buffer);
					client.set_send_response();

					// Cleanup
					remove_from_poll(cgi.pipe_from_cgi);
					cgi_processes.erase(fd);
			}
	}

	// Check timeout
	if (time(NULL) - cgi.start_time > 30)
	{
			kill(cgi.pid, SIGKILL);
			waitpid(cgi.pid, NULL, 0);
			// Send 504 Gateway Timeout to client
			cleanup_cgi(cgi);
	}


	*/
