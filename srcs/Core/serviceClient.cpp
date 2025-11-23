#include "service.hpp"


void Service::service_reading(std::vector<struct pollfd> &poll_fds, int i)
{
	std::cout << "\n###################################################" << std::endl;
	std::cout << "################## READING #######################" << std::endl;
	std::cout << "###################################################\n"
			  << std::endl;

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




	// 	if (clients[poll_fds[i].fd].handle_read())
	// 	if (clients[poll_fds[i].fd].can_i_create_response() || clients[poll_fds[i].fd].can_i_process_request())
	// 		poll_fds[i].events = POLLOUT;
	// 	else
	// 		handle_disconnection(poll_fds, i);
	// else
	// {
	// 	std::cout << "Service ========>>  Client work request " << clients[poll_fds[i].fd].can_i_process_request() << std::endl;
	// 	if (clients[poll_fds[i].fd].can_i_process_request() == true)
	// 		poll_fds[i].events = POLLOUT;
	// }
}

void Service::service_processing(std::vector<struct pollfd> &poll_fds, int i)
{
	std::cout << "\n###################################################" << std::endl;
	std::cout << "################## PROCESSING #######################" << std::endl;
	std::cout << "###################################################\n"
			  << std::endl;

	std::cout << "sttus: " << clients[poll_fds[i].fd].get_status_code() << std::endl;

	if (clients[poll_fds[i].fd]._response.get_response_data_full().size() > 8192)
		clients[poll_fds[i].fd].set_status_code(500);

	if (clients[poll_fds[i].fd].get_status_code() >= 500)
	{
		clients[poll_fds[i].fd].set_flags_error();
		clients[poll_fds[i].fd]._request._body = "";
	}
	std::cout << "sttus: " << clients[poll_fds[i].fd].get_status_code() << std::endl;
	if (clients[poll_fds[i].fd].get_status_code() < 300)
		clients[poll_fds[i].fd].process_request();
	clients[poll_fds[i].fd].set_create_response();
	std::cout << "sttus: " << clients[poll_fds[i].fd].get_status_code() << std::endl;
}

int Service::service_writing(std::vector<struct pollfd> &poll_fds, int i)
{
	std::cout << "\n###################################################" << std::endl;
	std::cout << "################## WRITING #######################" << std::endl;
	std::cout << "###################################################\n"
			  << std::endl;

	if (clients[poll_fds[i].fd].handle_write())
	{
		std::cout << "\n >> returning from inside writing - handle connection should happen outside << " << std::endl;
		handle_connection(poll_fds, i);
		// handle_connection(poll_fds, i);
		poll_fds[i].events = POLLIN;
		return (1);
	}
	else
	{
		std::cout << "Service ========>>  Client send response: " << clients[poll_fds[i].fd].can_i_send_response() << std::endl;
		if (!clients[poll_fds[i].fd].can_i_send_response())
		{
			std::cout << "[Writing] Finishing request, preparing client for next or closing" << std::endl;
			poll_fds[i].events = POLLIN;
			if (!clients[poll_fds[i].fd].can_i_close_connection())
				clients[poll_fds[i].fd].set_read_header();
			handle_connection(poll_fds, i);
		}
	}
	return (0);
}
