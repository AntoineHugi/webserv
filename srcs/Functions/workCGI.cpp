# include "workCGI.hpp"

void run_cgi(Client& client)
{
	(void)client;
	std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^  I am the GCI hook" << std::endl;

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
