#include <iostream>
#include "service.hpp"
#include <sys/socket.h>
#include <poll.h>

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



















int fill_request_data(int fd, Request &req)
{
	std::cout << "\033[33mClient is sending data \033[0m" << std::endl;
	char buffer[80]; // char buffer[8192];
	int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes_read > 0)
	{
		req._request_data.append(buffer, bytes_read);
		if (!req._header_parsed)
		{
			std::cout << "\033[31m  Header not parsed \033[0m"<<std::endl;
			if (req._request_data.size() > 16384)
			{
				req._status_code = 431;
				return 1;
			}

			if(req._request_data.find("\r\n\r\n") != std::string::npos)
			{
				req._header = req._request_data.substr(0, req._request_data.find("\r\n\r\n") + 4);
				std::cout << "\033[34mHeader: '" << req._header << "----\n" << "'\033[0m"<<std::endl;
				// parse_header();
				std::cout << "\033[35m  Header parsed \033[0m"<<std::endl;

				if (req._content_length > 300)  // e.g., 1 GB limit
				{
				   req._status_code = 431;
						return 1;
				}
				// REMOVE THIS AFTER PARSE HEADER IS DONE
				req._content_length = 323;
				req._header_parsed = true;
			}
		}

		if(req._header_parsed && req._content_length > 0)
		{
			if (req._request_data.size() >= req._header.size() + req._content_length)
			{
				req._body = req._request_data.substr(req._header.size(), req._content_length);
				std::cout << "\033[35mBody: '" << req._body << "----\n" << "'\033[0m"<<std::endl;
				// parse_body();
				// REMOVE THIS AFTER PARSE BODY IS DONE
				req._work_request = true;
			}
		}
	}
	else
	{
		std::cout << "Client " << fd << " nothing else to read or failure in reading." << std::endl;
		req._status_code = 431;
		return 1;
	}
	return 0;
}

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
			clients[client_fd] = Request();
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
		poll(poll_fds.data(), poll_fds.size(), -1);
		int new_client = 0;

		for (size_t i = 0; i < poll_fds.size(); i++)
		{
			if (poll_fds[i].revents == 0)
				continue;
			for (size_t j = 0; j < server_fds.size(); j++)
			{
				if (poll_fds[i].fd == server_fds[j].fd)
				{
					new_client = 1;
					break;
				}
			}

			if (new_client)
				add_client_to_polls(poll_fds, server_fds, this->clients, i);
			else
			{
				if(fill_request_data(poll_fds[i].fd, this->clients[poll_fds[i].fd]))
				{
					close(poll_fds[i].fd);
					poll_fds.erase(poll_fds.begin() + i);
					clients.erase(poll_fds[i].fd);
					std::cout << "Client " << poll_fds[i].fd << " disconnected, show status here: "<< clients[poll_fds[i].fd]._status_code <<" ." << std::endl;
					continue;
				}
				if (this->clients[poll_fds[i].fd]._work_request == true)
				{
					// Process the request here (not implemented in this snippet)
					// After processing, close the connection
					close(poll_fds[i].fd);
					poll_fds.erase(poll_fds.begin() + i);
					clients.erase(poll_fds[i].fd);
					std::cout << "Client " << poll_fds[i].fd << " SHOULD RECEIVE RESPONSE - NOT CODED YET ." << std::endl;
					continue;
				}
			}
		}
	}
}
