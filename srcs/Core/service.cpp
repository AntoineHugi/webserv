#include "service.hpp"
#include "serviceUtils.cpp"
#include "serviceClient.cpp"
#include "serviceCGI.cpp"
#include "serviceHandleConnection.cpp"

#include <iostream>
#include <sys/socket.h>
#include <cerrno>
#include <poll.h>
#include <cstring>
#include <stdio.h>
#include <errno.h>

Service::Service() : servers() {}

Service::Service(const Service &other)
{
	(void)other;
}

Service &Service::operator=(const Service &other)
{
	(void)other;
	return (*this);
}

Service::~Service() {}

volatile sig_atomic_t g_shutdown = 0;

void handle_shutdown(int sig)
{
	if (sig == SIGINT || sig == SIGTERM)
	{
		std::cout << "\nShutdown signal received..." << std::endl;
		g_shutdown = 1;
	}
}

/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/

void Service::poll_service()
{
	this->set_polls();

	while (g_shutdown == 0)
	{
		// Use timeout 0 if any client has leftover data to process
		int timeout = 60000;
		for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			if (it->second.leftover_chunk())
			{
				timeout = 0;
				break;
			}
		}
		int ret = poll(this->fds["poll_fds"].data(), this->fds["poll_fds"].size(), timeout);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue; // Interrupted, retry
			perror("poll failed");
			break;
		}

		for (int i = this->fds["poll_fds"].size() - 1; i >= 0; i--)
		{

			int server_fd_if_new_client = server_fd_for_new_client(this->fds["poll_fds"][i].fd , this->fds["server_fds"]);
			int cgi_fd_if_cgi = cgi_fd_for_cgi(this->fds["poll_fds"][i].fd , this->fds["cgi_fds"]);

			if (this->fds["poll_fds"][i].revents == 0)
			{
				if (server_fd_if_new_client != -1 || (server_fd_if_new_client == -1 && clients[this->fds["poll_fds"][i].fd].leftover_chunk() == false))
					continue;
				else if (clients[this->fds["poll_fds"][i].fd].is_inactive() && this->fds["poll_fds"][i].revents & POLLIN)
					handle_disconnection(this->fds["poll_fds"], i);
			}
			if (server_fd_if_new_client != -1)
			{
				add_client_to_polls(this->fds["poll_fds"], this->clients, this->fds["poll_fds"][i].fd, this->servers[server_fd_if_new_client]);
				std::cout << "\033[32m New client connected. Total clients: " << (this->fds["poll_fds"].size() - this->fds["server_fds"].size()) << "\033[0m" << std::endl;
			}
			else if(cgi_fd_if_cgi != -1)
				cgi_handler(i);
			else
			{
				Client &client = clients[this->fds["poll_fds"][i].fd];
				client.update_last_interaction();
				if (client.leftover_chunk() == false && this->fds["poll_fds"][i].revents & (POLLERR | POLLHUP | POLLNVAL))
				{
					std::cout << "this->fds['poll_fds'][i].revents : " << this->fds["poll_fds"][i].revents << std::endl;
					handle_disconnection(this->fds["poll_fds"], i);
					continue;
				}
				if ((this->fds["poll_fds"][i].revents & POLLIN || client.leftover_chunk()) && (client.can_i_read_header() == true || client.can_i_read_body() == true))
					service_reading(this->fds["poll_fds"], i);
				if (i < (int)this->fds["poll_fds"].size() && (this->fds["poll_fds"][i].revents & POLLOUT) && client.can_i_process_request() == true)
					service_processing(this->fds["poll_fds"], i);
				if (i < (int)this->fds["poll_fds"].size() && (this->fds["poll_fds"][i].revents & POLLOUT) && (client.can_i_create_response() == true || client.can_i_send_response() == true))
					service_writing(this->fds["poll_fds"], i);
			}
		}
	}
	std::cout << "Shutting down gracefully..." << std::endl;
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		close(it->first);
	for (size_t i = 0; i < servers.size(); i++)
		close(servers[i].get_sock());
}
