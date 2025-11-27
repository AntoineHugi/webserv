#include "service.hpp"

void Service::cgi_handler(int i)
{
	std::cout << "\n###################################################" << std::endl;
	std::cout << "################## CGI REQUEST #######################" << std::endl;
	std::cout << "###################################################\n"
			  << std::endl;

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
			remove_fd(cgi.get_pipe_to_cgi());
			cgi.set_processing_and_writing();
		}
		else
			cgi.update_bytes_written(bytes_sent);
	}
	else if (fds["poll_fds"][i].fd == cgi.get_pipe_from_cgi() && cgi.can_i_process_and_write())
	{
		char buffer[1024];
		ssize_t n = read(cgi.get_pipe_from_cgi(), buffer, sizeof(buffer));

		if (n > 0)
			cgi.append_to_output(buffer, n);
		else if (n == 0)  // EOF
		{
			int status;
			waitpid(cgi.get_pid(), &status, 0);
			client._response.set_response_data(cgi.get_output_buffer());

		// 	// Parse CGI output and send to client
		// 	Client& client = clients[cgi._client_fd];
		// 	parse_cgi_output(cgi._output_buffer, client);
		// 	client.set_send_response();
			// Cleanup
			remove_fd(cgi.get_pipe_from_cgi());
			cgi.set_finish();
			client.set_create_response();
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
		char path_buf[PATH_MAX];

		// Get the current executable path
		ssize_t count = readlink("/proc/self/exe", path_buf, PATH_MAX);
		if (count == -1) {
				perror("Error getting executable path");
				return;
		}

		// Null-terminate the string
		path_buf[count] = '\0';

		// Get the directory part from the full path
		std::string dir_path = std::string(path_buf);
		dir_path = dir_path.substr(0, dir_path.find_last_of("/"));  // Get the directory

		// Construct the full path for the command to execute
		std::string exec_path = dir_path + "/cgi/test.py";

		std::string arg0 = "/usr/bin/python3";
		char *argv[] = {strdup(arg0.c_str()), strdup(exec_path.c_str()), NULL };
		char *envp[] = {NULL};
		execve(argv[0], argv, envp);
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
