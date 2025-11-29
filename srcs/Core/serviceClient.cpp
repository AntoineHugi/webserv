#include "service.hpp"


void Service::service_reading(std::vector<struct pollfd> &poll_fds, int i)
{
	if (DEBUG)
	{
		std::cout << "\n###################################################" << std::endl;
		std::cout << "################## READING #######################" << std::endl;
		std::cout << "###################################################\n" << std::endl;
	}

	int read_status = clients[poll_fds[i].fd].handle_read();
	if (read_status == 0)
		return;
	else if (read_status == 1)
	{
		if (clients[poll_fds[i].fd].can_i_create_response())
			poll_fds[i].events = POLLOUT;
		else
			handle_connection(poll_fds, i);
	}
	else
		poll_fds[i].events = POLLOUT;
}

void Service::service_processing(std::vector<struct pollfd> &poll_fds, int i)
{
	if (DEBUG)
	{
		std::cout << "\n###################################################" << std::endl;
		std::cout << "################## PROCESSING #######################" << std::endl;
		std::cout << "###################################################\n" << std::endl;
	}

	if (clients[poll_fds[i].fd]._request._isCGI && clients[poll_fds[i].fd].get_status_code() < 300) // TODO: check status too otherwise infinite loop
	{
		if (DEBUG)
			std::cout << "will create CGI processes " << std::endl;
		if (clients[poll_fds[i].fd].can_i_process_request())
		{
			this->setup_cgi_request(i);
			clients[poll_fds[i].fd].set_wait_cgi();
		}
		else if (clients[poll_fds[i].fd].am_i_waiting_cgi())
		{
			if ((*cgi_processes[cgi_fd_for_cgi(this->fds["poll_fds"][i].fd , this->fds["cgi_fds"])]).am_i_finish())
				clients[poll_fds[i].fd].can_i_create_response();
		}
		return;
	}
	else
	{
		clients[poll_fds[i].fd].process_request();
		clients[poll_fds[i].fd].set_create_response();
	}

}

int Service::service_writing(std::vector<struct pollfd> &poll_fds, int i)
{
	if (DEBUG)
	{
		std::cout << "\n###################################################" << std::endl;
		std::cout << "################## WRITING #######################" << std::endl;
		std::cout << "###################################################\n"<< std::endl;
	}
	if (clients[poll_fds[i].fd].handle_write())
	{
		std::cout << "\n >> returning from inside writing - handle connection should happen outside << " << std::endl;
		handle_connection(poll_fds, i);
		poll_fds[i].events = POLLIN;
		return (1);
	}
	else
	{
		if (DEBUG)
			std::cout << "Service ========>>  Client send response: " << clients[poll_fds[i].fd].can_i_send_response() << std::endl;
		if (!clients[poll_fds[i].fd].can_i_send_response())
		{
			if (DEBUG)
				std::cout << "[Writing] Finishing request, preparing client for next or closing" << std::endl;
			poll_fds[i].events = POLLIN;
			if (!clients[poll_fds[i].fd].can_i_close_connection())
				clients[poll_fds[i].fd].set_read_header();
			handle_connection(poll_fds, i);
		}
	}
	return (0);
}
