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



















void fill_request_data(std::vector<struct pollfd> &poll_fds, std::map<int, Request> &clients, size_t i)
{
	std::cout << "\033[33mClient is sending data \033[0m" << std::endl;

	// char buffer[80]; // test to trigger 431 error
	char buffer[8192];

	// recv(fd, buffer, length, flags)
	int bytes_read = recv(poll_fds[i].fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes_read > 0)
	{
		// if (this->clients[poll_fds[i].fd]._request_data.size() + bytes_read > 160) // test to trigger 431 error
		if (clients[poll_fds[i].fd]._request_data.size() + bytes_read > 16384 || clients[poll_fds[i].fd]._request_data.size() < clients[poll_fds[i].fd]._content_length)
		{
			close(poll_fds[i].fd);
			poll_fds.erase(poll_fds.begin() + i);
			clients.erase(poll_fds[i].fd);
			std::cout << "Client " << poll_fds[i].fd << " disconnected -431 Request Header Fields Too Large." << std::endl;
			// TODO: Send response 431 Request Header Fields Too Large
			return;
		}

		clients[poll_fds[i].fd]._request_data.append(buffer, bytes_read);
		if(!clients[poll_fds[i].fd]._header_parsed && clients[poll_fds[i].fd]._request_data.find("\r\n\r\n") != std::string::npos)
		{
			std::cout << "Full request received from client " << poll_fds[i].fd << ":\n-----\n" << clients[poll_fds[i].fd]._request_data << "----\n" << std::endl;
			clients[poll_fds[i].fd]._header_parsed = true;
			// parse_header();
		}
		{
			clients[poll_fds[i].fd]._header = clients[poll_fds[i].fd]._request_data.substr(0, clients[poll_fds[i].fd]._request_data.find("\r\n\r\n") + 4);
			std::cout << "\033[34mHeader: '" << clients[poll_fds[i].fd]._header << "----\n" << "'\033[0m"<<std::endl;
			// parse_header();
		}

		if(clients[poll_fds[i].fd]._header_parsed && clients[poll_fds[i].fd]._request_data.size() == clients[poll_fds[i].fd]._header.size() + clients[poll_fds[i].fd]._content_length)
		{
			clients[poll_fds[i].fd]._body = clients[poll_fds[i].fd]._request_data.substr(clients[poll_fds[i].fd]._header.size(), clients[poll_fds[i].fd]._content_length);
			std::cout << "\033[35mBody: '" << clients[poll_fds[i].fd]._body << "----\n" << "'\033[0m"<<std::endl;
			// parse_body();
		}

	}
	else
	{
		close(poll_fds[i].fd);
		poll_fds.erase(poll_fds.begin() + i);
		clients.erase(poll_fds[i].fd);
		std::cout << "Client " << poll_fds[i].fd << " nothing else to read or failure in reading." << std::endl;
		// TODO: Send response 431 Request Header Fields Too Large
		return;
	}
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
				fill_request_data();
				close(poll_fds[i].fd);
				poll_fds.erase(poll_fds.begin() + i);
				clients.erase(poll_fds[i].fd);
				std::cout << "Client " << poll_fds[i].fd << " disconnected after request." << std::endl;


				std::cout << "\033[33mClient is sending data \033[0m" << std::endl;


				// char buffer[80]; // test to trigger 431 error
				char buffer[8192];

				// recv(fd, buffer, length, flags)
				int bytes_read = recv(poll_fds[i].fd, buffer, sizeof(buffer) - 1, 0);

				if (bytes_read > 0)
				{
					// if (this->clients[poll_fds[i].fd]._request_data.size() + bytes_read > 160) // test to trigger 431 error
					if (this->clients[poll_fds[i].fd]._request_data.size() + bytes_read > 16384)
					{
						close(poll_fds[i].fd);
						poll_fds.erase(poll_fds.begin() + i);
						this->clients.erase(poll_fds[i].fd);
						std::cout << "Client " << poll_fds[i].fd << " disconnected -431 Request Header Fields Too Large." << std::endl;
						// TODO: Send response 431 Request Header Fields Too Large
						continue;
					}

					this->clients[poll_fds[i].fd]._request_data.append(buffer, bytes_read);
					if(this->clients[poll_fds[i].fd]._request_data.find("\r\n\r\n") != std::string::npos)
					{
						std::cout << "Full request received from client " << poll_fds[i].fd << ":\n-----\n" << this->clients[poll_fds[i].fd]._request_data << "----\n" << std::endl;
						// parse_header();

						// Here you would normally process the request and send a response
						// For now, we just close the connection
						this->clients[poll_fds[i].fd]._header = this->clients[poll_fds[i].fd]._request_data.substr(0, this->clients[poll_fds[i].fd]._request_data.find("\r\n\r\n") + 4);
						std::cout << "\033[34mHeader: '" << this->clients[poll_fds[i].fd]._header << "----\n" << "'\033[0m"<<std::endl;
						close(poll_fds[i].fd);
						poll_fds.erase(poll_fds.begin() + i);
						this->clients.erase(poll_fds[i].fd);
						std::cout << "Client " << poll_fds[i].fd << " disconnected after request." << std::endl;
					}
				}
				else
				{
					close(poll_fds[i].fd);
					poll_fds.erase(poll_fds.begin() + i);
					this->clients.erase(poll_fds[i].fd);
					std::cout << "Client " << poll_fds[i].fd << " nothing else to read or failure in reading." << std::endl;
					// TODO: Send response 431 Request Header Fields Too Large
					continue;
				}

			}
		}

		//check each fd and see if there's any new data in it
		// if there is, decide:
			// is it a server fd? ==> that means a new client has made a connection
				// 'accept' the connection and add the new fd to the pollfd vector
			// else ==> a client that was already connected is sending the data
				// read the information with 'recv' and save the bytes read
				// we need to create a buffer to keep reading in the next iteration
				// thus, might be interesting creating a Client class, or at least a struct with some information of the current reading state
				// after everything was read ==> prepare http answer => answer the client with 'send' ==> close the client's connection and remove it from the fds vector
		// If there is nothing new, just continue the loop

	}
}
