#include "service.hpp"
#include <iostream>
#include "service.hpp"
#include <sys/socket.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include <stdio.h>
#include <errno.h>


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
				parse_header(req);
				std::cout << "\033[35m  Header parsed \033[0m"<<std::endl;
				if (req._content_length > 400)  // e.g., 1 GB limit
				{
					req._status_code = 431;
					return 1;
				}
			}
		}

		if(req._header_parsed == true && req._content_length == 0)
			req._work_request = true;
		if(req._header_parsed == true && req._content_length > 0)
		{
			if (req._request_data.size() >= req._header.size() + req._content_length)
			{
				req._body = req._request_data.substr(req._header.size(), req._content_length);
				std::cout << "\033[35mBody: '" << req._body << "----\n" << "'\033[0m"<<std::endl;
				parse_body(req);
			}
		}

	}
	else if (bytes_read == 0)
	{
			std::cout << "Client " << fd << " closed connection" << std::endl;
			return 1;  // Signal to close
	}
	else
	{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
					return 0;  // No data available, try again later

			// Actual error
			std::cerr << "recv error: " << strerror(errno) << std::endl;
			return 1;  // Signal to close
	}
	return 0;
}

void handle_read(std::map<int, Request> &clients, std::vector<struct pollfd> &poll_fds, size_t i)
{
	if (clients[poll_fds[i].fd]._inORout == true)
	{
		std::cout << "==>>  Client is waiting for answer" << std::endl;
		return;
	}
	if(fill_request_data(poll_fds[i].fd, clients[poll_fds[i].fd]))
	{
		std::cout << "==>>  Client will be closed" << std::endl;
		int fd = poll_fds[i].fd;
		close(fd);
		poll_fds.erase(poll_fds.begin() + i);
		clients.erase(fd);
		std::cout << "Client " << poll_fds[i].fd << " disconnected, show status here: "<< clients[poll_fds[i].fd]._status_code <<" ." << std::endl;
		return;
	}
	if (clients[poll_fds[i].fd]._work_request == true)
	{
		process_request(clients[poll_fds[i].fd]);
		if(clients[poll_fds[i].fd]._status_code != 200)
		{
			std::cout << "==>>  Client will be closed" << std::endl;
			int fd = poll_fds[i].fd;
			close(fd);
			poll_fds.erase(poll_fds.begin() + i);
			clients.erase(fd);
		}
		else
		{
			clients[poll_fds[i].fd]._inORout = true;
			poll_fds[i].events = POLLOUT;
		}
	}
	return;
}
void handle_write(std::map<int, Request> &clients, std::vector<struct pollfd> &poll_fds, size_t i)
{
	std::cout << "==>>  Client will receive answer" << std::endl;

	std::string res = format_response(clients[poll_fds[i].fd]);
	std::cout << "Response to be sent:\n-----\n" << res << std::endl;

	ssize_t bytes_sent = send(poll_fds[i].fd, res.c_str(), strlen(res.c_str()), 0);
	if (bytes_sent == -1) {
			std::cerr << "Error sending response" << std::endl;
	}

	// KEY DECISION POINT:
	if (clients[poll_fds[i].fd]._should_keep_alive == true)
	{
		// Keep connection open for next request
		poll_fds[i].events = POLLIN;
		clients[poll_fds[i].fd]._inORout = false;
		flush_request_data(clients[poll_fds[i].fd]);
		std::cout << "Connection kept alive for fd: " << poll_fds[i].fd << std::endl;
	}
	else
	{
		// Close connection
		std::cout << "Closing connection (sent Connection: close)" << std::endl;
		int fd = poll_fds[i].fd;
		close(fd);
		poll_fds.erase(poll_fds.begin() + i);
		clients.erase(fd);
		std::cout << "\033[32m Client disconnected. Total clients (with server): " << (poll_fds.size()) << "\033[0m" << std::endl;

	}
}
void handle_disconnect(std::map<int, Request> &clients, std::vector<struct pollfd> &poll_fds, size_t i)
{
	(void)i;
	(void)clients;
	(void)poll_fds;
}
