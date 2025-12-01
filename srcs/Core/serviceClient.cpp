#include "service.hpp"

void Service::service_reading(std::vector<struct pollfd> &poll_fds, int i)
{
	print_header("READING");
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
	print_header("PROCESSING");
	Client &client = clients[poll_fds[i].fd];

	client.update_last_interaction();
	if (client.can_i_parse_body() == true)
	{
		int check = client.try_parse_body();
		if (check == 1)
		{
			if (client.can_i_create_response())
				return;
			else
				handle_connection(poll_fds, i);
		}
		else if (check == 2)
			client.set_process_request();
		else
			return;
	}
	else if (client._request._is_cgi() && client.get_status_code() < 300)
	{
		print_white(">>> This client will create CGI processes and wait", DEBUG);
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
	print_header("WRITING");
	Client &client = clients[poll_fds[i].fd];

	client.update_last_interaction();
	if (client.handle_write())
	{
		print_yellow("\n >> returning from inside writing - handle connection should happen outside << ", true);
		handle_connection(poll_fds, i);
		poll_fds[i].events = POLLIN;
		return (1);
	}
	else
	{
		print_cyan("Service ========>>  Client send response: " + convert_to_string(client.can_i_send_response()), DEBUG);
		if (!client.can_i_send_response())
		{
			print_blue("[Writing] Finishing request, preparing client for next or closing", DEBUG);
			poll_fds[i].events = POLLIN;
			if (!client.can_i_close_connection())
				client.set_read_header();
			handle_connection(poll_fds, i);
		}
	}
	return (0);
}
