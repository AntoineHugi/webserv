#include <iostream>
#include "service.hpp"
#include "request_in.cpp"
#include <sys/socket.h>
#include <cerrno>
#include <poll.h>
#include <cstring>
#include <stdio.h>
#include <errno.h>

Service::Service(): servers() {}

Service::Service(const Service& other)
{
	(void)other;
}

Service& Service::operator=(const Service& other)
{
	(void)other;
	return (*this);
}

Service::~Service() {}



void add_client_to_polls(std::vector<struct pollfd> &poll_fds, std::vector<struct pollfd> &server_fds, std::map<int, Request> &clients, size_t i)
{
	std::cout << "\033[32m New connection! \033[0m" << std::endl;

	int client_fd = accept(poll_fds[i].fd, NULL, NULL);
	// TODO: check if accept was sucessful
	if (client_fd >= 0)
	{
			struct pollfd client_pfd;
			client_pfd.fd = client_fd;
			client_pfd.events = POLLIN;  // Wait for data from client
			client_pfd.revents = 0;
			poll_fds.push_back(client_pfd);
			clients.insert(std::pair<int, Request>(client_fd, Request()));
			std::cout << "\033[32m Client " << client_fd << " connected. Total clients: " << (poll_fds.size() - server_fds.size()) << "\033[0m" << std::endl;
	}
}

void set_polls(std::vector<struct pollfd> &poll_fds, std::vector<struct pollfd> &server_fds, std::vector<Server> &servers)
{
	for (size_t i = 0; i < servers.size(); i++)
	{
		struct pollfd pfd;
		pfd.fd = servers[i].get_sock();
		pfd.events = POLLIN;
		poll_fds.push_back(pfd);
		server_fds.push_back(pfd);
	}
}

void Service::poll_service()
{
	std::vector<struct pollfd> poll_fds;
	std::vector<struct pollfd> server_fds;
	set_polls(poll_fds, server_fds, this->servers);

	while(true)
	{
		// poll(pointet to the first elemnt type struct pollfd, number of fds, timeout)
		int ret = poll(poll_fds.data(), poll_fds.size(), 60000);
		if (ret < 0) {
				if (errno == EINTR) continue;  // Interrupted, retry
				perror("poll failed");
				break;
		}
		for (int i = poll_fds.size() - 1; i >= 0; i--)
		{
			bool new_client = false;
			if (poll_fds[i].revents == 0)
				continue;
			for (size_t j = 0; j < server_fds.size(); j++)
			{
				if (poll_fds[i].fd == server_fds[j].fd)
				{
					new_client = true;
					break;
				}
			}
			if (new_client)
				add_client_to_polls(poll_fds, server_fds, this->clients, i);
			else
			{
					// Check for errors/hangup first
					std::cout << "Handling event for client fd: " << poll_fds[i].fd << std::endl;
					std::cout << " with poll_fds[i].revents: " << poll_fds[i].revents << std::endl;
					std::cout << " with clients[poll_fds[i].fd]._inORout: " << this->clients[poll_fds[i].fd]._inORout << std::endl;
					if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
					{
							handle_disconnect(this->clients, poll_fds, i);
							continue;
					}
					if (poll_fds[i].revents & POLLIN)
					{
						std::cout << "===========>>  Client will send request" << std::endl;
						handle_read(this->clients, poll_fds, i);
					}
					if (i < (int)poll_fds.size() && (poll_fds[i].revents & POLLOUT))
							handle_write(this->clients, poll_fds, i);
			}
		}
	}
}
