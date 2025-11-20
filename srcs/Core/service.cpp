#include "service.hpp"
#include "serviceUtils.cpp"
#include "serviceClient.cpp"
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
	std::vector<struct pollfd> poll_fds;
	std::vector<struct pollfd> server_fds;
	std::vector<struct pollfd> cgi_fds;
	set_polls(poll_fds, server_fds, this->servers);

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
		int ret = poll(poll_fds.data(), poll_fds.size(), timeout);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue; // Interrupted, retry
			perror("poll failed");
			break;
		}
		for (int i = poll_fds.size() - 1; i >= 0; i--)
		{
			bool new_client = false;
			int server_index;
			for (size_t j = 0; j < server_fds.size(); j++)
			{
				if (poll_fds[i].fd == server_fds[j].fd)
				{
					server_index = j;
					new_client = true;
					break;
				}
			}
			if (poll_fds[i].revents == 0)
			{
				if (new_client || (!new_client && clients[poll_fds[i].fd].leftover_chunk() == false))
					continue;
				else if (clients[poll_fds[i].fd].is_inactive() && poll_fds[i].revents & POLLIN)
					handle_disconnection(poll_fds, i);
			}
			if (new_client)
			{
				add_client_to_polls(poll_fds, this->clients, i, this->servers[server_index]);
				std::cout << "\033[32m New client connected. Total clients: " << (poll_fds.size() - server_fds.size()) << "\033[0m" << std::endl;
			}
			else
			{
				Client &client = clients[poll_fds[i].fd];
				client.update_last_interaction();
				if (client.leftover_chunk() == false && poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
				{
					std::cout << "poll_fds[i].revents : " << poll_fds[i].revents << std::endl;
					handle_disconnection(poll_fds, i);
					continue;
				}
				if ((poll_fds[i].revents & POLLIN || client.leftover_chunk()) && (client.can_i_read_header() == true || client.can_i_read_body() == true))
					service_reading(poll_fds, i);
				if (i < (int)poll_fds.size() && (poll_fds[i].revents & POLLOUT) && client.can_i_process_request() == true)
					service_processing(poll_fds, i);
				if (i < (int)poll_fds.size() && (poll_fds[i].revents & POLLOUT) && (client.can_i_create_response() == true || client.can_i_send_response() == true))
					service_writing(poll_fds, i);
			}
		}
	}
	std::cout << "Shutting down gracefully..." << std::endl;
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		close(it->first);
	for (size_t i = 0; i < servers.size(); i++)
		close(servers[i].get_sock());
}
