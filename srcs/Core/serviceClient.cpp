#include "service.hpp"


void Service::service_reading(std::vector<struct pollfd> &poll_fds, int i)
{
	if (DEBUG)
	{
		std::cout << "\n###################################################" << std::endl;
		std::cout << "################## READING #######################" << std::endl;
		std::cout << "###################################################\n" << std::endl;
	}
	Client &client = clients[poll_fds[i].fd];

	client.update_last_interaction();
	int read_status = clients[poll_fds[i].fd].handle_read();
	if (read_status == 0)
		return;
	else if (read_status == 1)
	{
		if (client.can_i_create_response())
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

	Client &client = clients[poll_fds[i].fd];

	client.update_last_interaction();
	if (client._request._isCGI && client.get_status_code() < 300) // TODO: check status too otherwise infinite loop
	{
		if (DEBUG)
			std::cout << "will create CGI processes " << std::endl;
		if (client.can_i_process_request())
		{
			this->setup_cgi_request(i);
			client.set_wait_cgi();
		}
		else if (client.am_i_waiting_cgi())
		{
			if ((*cgi_processes[cgi_fd_for_cgi(this->fds["poll_fds"][i].fd , this->fds["cgi_fds"])]).am_i_finish())
				client.can_i_create_response();
		}
		return;
	}
	else
	{
		client.process_request();
		client.set_create_response();
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

	Client &client = clients[poll_fds[i].fd];

	client.update_last_interaction();
	if (client.handle_write())
	{
		std::cout << "\n >> returning from inside writing - handle connection should happen outside << " << std::endl;
		handle_connection(poll_fds, i);
		poll_fds[i].events = POLLIN;
		return (1);
	}
	else
	{
		if (DEBUG)
			std::cout << "Service ========>>  Client send response: " << client.can_i_send_response() << std::endl;
		if (!client.can_i_send_response())
		{
			if (DEBUG)
				std::cout << "[Writing] Finishing request, preparing client for next or closing" << std::endl;
			poll_fds[i].events = POLLIN;
			if (!client.can_i_close_connection())
				client.set_read_header();
			handle_connection(poll_fds, i);
		}
	}
	return (0);
}
