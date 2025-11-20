# include "workCGI.hpp"

void run_cgi(Client& client)
{
	(void)client;
	std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^  I am the GCI hook" << std::endl;

	// Child:
		dup2(pipe_to_cgi[0], STDIN_FILENO);
		// dup2(pipe_from_cgi[1], STDOUT_FILENO);
		execve("/usr/bin/php-cgi", argv, envp);

	// Find the file in the CGI folder
	  // for now, I'll assume the file exists


		/*
		1. Client sends POST /script.php + body
			State: RECEIVING_REQUEST

		2. Request complete, detect CGI
			State: WAITING_FOR_CGI

		3. fork(), setup pipes, add to poll()
			Parent returns to event loop

		4. [If POST] Event loop: pipe_to_cgi writable
			write() body to CGI stdin (might take multiple iterations)
			When done: close(pipe_to_cgi[1])

		5. [Meanwhile] Other clients being served

		6. Event loop: pipe_from_cgi readable
			read() CGI output (partial)
			Accumulate in cgi_output string

		7. read() returns 0 (EOF)
			CGI finished
			waitpid() to reap child

		8. Parse CGI output (headers + body)
			Build HTTP response
			Client state: SENDING_RESPONSE

		9. Event loop: client_fd writable
			send() response to client

		10. Response complete
				close() or keep-alive
	*/
	// If found, run the file and save the response




	// it needs to answer back to the server while the CGI program runs
}


void add_cgi_to_polls(std::vector<struct pollfd> &cgi_fds)
{
	std::cout << "\n###################################################"<< std::endl;
	std::cout << "################## ADDING CGI #######################"<< std::endl;
	std::cout << "###################################################\n"<< std::endl;
	std::cout << "\033[32m New connection! \033[0m" << std::endl;

	int pipe_to_cgi[2];
	int pipe_from_cgi[2];

	if(pipe(pipe_to_cgi))
	{
		// TODO: handle error;
		return;
	}
	if(pipe(pipe_from_cgi))
	{
		// TODO: handle error;
		return;
	}

	struct pollfd ptc;
	ptc.fd = pipe_to_cgi[1];
	ptc.events = POLLIN | POLLOUT;
	ptc.revents = 0;
	cgi_fds.push_back(ptc);

	struct pollfd pfc;
	pfc.fd = pipe_from_cgi[1];
	pfc.events = POLLIN | POLLOUT;
	pfc.revents = 0;
	cgi_fds.push_back(pfc);
}
