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
		if (client.can_i_create_response() || client.can_i_process_request() || client.am_i_waiting_file())
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
	if (client.get_status_code() >= 400)
	{
		int status_code = client.get_status_code();
		if (!client.get_server()->get_error_page().empty() && !client.get_server()->get_error_page()[status_code].empty())
		{
			int fd = Method::get_file(client, client.get_server()->get_error_page()[status_code]);
			if (fd > 0)
			{
				add_poll_to_vectors(fd, (POLLIN | POLLOUT), "files_fds");
				files_fds.insert(std::make_pair(fd, &client));
				client.set_wait_file();
				return;
			}
		}
		client.create_default_error();
		client.set_create_response();
		return ;
	}
	else if (client.get_status_code() == 301 || client.get_status_code() == 302)
	{
		client.set_create_response();
		return;
	}
	else if (client._request._isCGI && client.get_status_code() < 300)
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
	else if (client.am_i_waiting_file())
		return;
	else
	{
		int fd = client.process_request();

		if (fd == -1)
			client.set_process_request();
		
		/* no file need, just continue (ex: delete request, or get directory) */
		else if (fd == 0)
			client.set_create_response();
		else
		{
			add_poll_to_vectors(fd, (POLLIN | POLLOUT), "files_fds");
			files_fds.insert(std::make_pair(fd, &client));
			client.set_wait_file();
		}
	}

}

int Service::service_writing(std::vector<struct pollfd> &poll_fds, int i)
{
	print_header("WRITING");
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
