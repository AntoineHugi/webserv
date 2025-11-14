#include "service.hpp"
#include <iostream>
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



void add_client_to_polls(std::vector<struct pollfd> &poll_fds, std::vector<struct pollfd> &server_fds, std::map<int, Client> &clients, size_t i, Server& server)
{
	std::cout << "\n###################################################"<< std::endl;
	std::cout << "################## ADDING CLIENT #######################"<< std::endl;
	std::cout << "###################################################\n"<< std::endl;
	std::cout << "\033[32m New connection! \033[0m" << std::endl;

	int client_fd = accept(poll_fds[i].fd, NULL, NULL);
	// TODO: check if accept was sucessful
	if (client_fd >= 0)
	{
			struct pollfd client_pfd;
			client_pfd.fd = client_fd;
			client_pfd.events = POLLIN | POLLOUT; // Wait for data from client
			client_pfd.revents = 0;
			poll_fds.push_back(client_pfd);
			clients.insert(std::pair<int, Client>(client_fd, Client(client_fd, server)));
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

	// int i = 0;
	while(true)
	{
		// i++ ;
		// if (i > 20)
		// 	break;
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
			if (poll_fds[i].revents == 0 && (new_client || (!new_client && this->clients[poll_fds[i].fd].leftover_chunk() == false)))
				continue;

			if (new_client)
				add_client_to_polls(poll_fds, server_fds, this->clients, i, this->servers[server_index]);
			else
			{
				// Check for errors/hangup first
				// std::cout << "Handling event for client fd: " << poll_fds[i].fd << std::endl;
				// std::cout << " with poll_fds[i].revents: " << poll_fds[i].revents << std::endl;
				// std::cout << " with clients[poll_fds[i].fd].get_state(): " << this->clients[poll_fds[i].fd].get_state() << std::endl;
				if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
				{
					this->handle_disconnection(poll_fds, i);
					continue; // might not need to continue
				}
				if (poll_fds[i].revents & POLLIN || this->clients[poll_fds[i].fd].leftover_chunk() == true)
				{
					std::cout << "\n###################################################"<< std::endl;
					std::cout << "################## READING #######################"<< std::endl;
					std::cout << "###################################################\n"<< std::endl;

					std::cout << "Service ========>>  Client will send request" << std::endl;
					if (this->clients[poll_fds[i].fd].handle_read())
						this->handle_connection(poll_fds, i);
					else
					{
						std::cout << "Service ========>>  Client work request " << this->clients[poll_fds[i].fd].can_i_process_request() <<  std::endl;
						if (this->clients[poll_fds[i].fd].can_i_process_request() == true)
								poll_fds[i].events = POLLOUT;
					}
				}
				if (i < (int)poll_fds.size() && (poll_fds[i].revents & POLLOUT) && this->clients[poll_fds[i].fd].can_i_process_request() == true)
				{
					std::cout << "\n###################################################"<< std::endl;
					std::cout << "################## WRITING #######################"<< std::endl;
					std::cout << "###################################################\n"<< std::endl;

					std::cout << "Service ========>>  Started processing request" << std::endl;
					// need to figure out how to avoid delays, for example cgi

					//process_request(clients[poll_fds[i].fd]);
					//ex: this->clients[poll_fds[i].fd]._server.croupier(clients[poll_fds[i].fd]);
					//if cgi -> fork() and get response (this should be in the process_request function)
					this->clients[poll_fds[i].fd]._response.set_response_data(this->clients[poll_fds[i].fd]._response.format_response(this->clients[poll_fds[i].fd].get_status_code(), this->clients[poll_fds[i].fd].should_keep_alive(), this->clients[poll_fds[i].fd]._request._header_kv["Version"], this->clients[poll_fds[i].fd]._request._body));
					this->clients[poll_fds[i].fd].handle_write();
					this->handle_connection(poll_fds, i);
				}
			}
		}
	}
}


void	Service::handle_connection(std::vector<struct pollfd> &poll_fds, const size_t& i)
{
	if (this->clients[poll_fds[i].fd].get_status_code() < 300)
	{
		if (this->clients[poll_fds[i].fd].should_keep_alive() == true)
		{
			std::string save_buffer = "";
			// Keep connection open for next request
			poll_fds[i].events = POLLIN;
			if (this->clients[poll_fds[i].fd].leftover_chunk() == true)
				save_buffer = this->clients[poll_fds[i].fd]._request._request_data.substr(this->clients[poll_fds[i].fd]._request._header.size() + this->clients[poll_fds[i].fd]._request._body.size());
			this->clients[poll_fds[i].fd].refresh_client();
			this->clients[poll_fds[i].fd]._request._request_data = save_buffer;
			std::cout << "Connection kept alive for fd: " << poll_fds[i].fd << std::endl;
		}
		else
		{
			// Close connection
			std::cout << "Closing connection (sent Connection: close)" << std::endl;
			int fd = poll_fds[i].fd;

			if(close(fd))
			{
				std::cerr << "Error closing client fd: " << fd << " Error: " << strerror(errno) << std::endl;
				return;
			}
			poll_fds.erase(poll_fds.begin() + i);
			clients.erase(fd);
			std::cout << "\033[32m Client disconnected. Total clients (with server): " << (poll_fds.size()) << "\033[0m" << std::endl;
		}
	}
	else
	{
		std::cout << "==>>  Client will be closed" << std::endl;
		int fd = poll_fds[i].fd;
		int status = this->clients[poll_fds[i].fd].get_status_code();
		if(close(fd))
		{
			std::cerr << "Error closing client fd: " << fd << " Error: " << strerror(errno) << std::endl;
			return;
		}
		poll_fds.erase(poll_fds.begin() + i);
		clients.erase(fd);
		std::cout << "Client " << fd << " disconnected, show status here: "<< status <<" ." << std::endl;
		return;
	}
}

void	Service::handle_disconnection(std::vector<struct pollfd> &poll_fds, const size_t& i)
{
	(void)i;
	(void)poll_fds;
	// TODO: clean client data (FDs, memory, etc)
	std::cout << "Handling disconnection (error/hangup) for fd: " << poll_fds[i].fd << std::endl;
}
