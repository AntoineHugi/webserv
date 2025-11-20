#include "service.hpp"
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

void add_client_to_polls(std::vector<struct pollfd> &poll_fds, std::vector<struct pollfd> &server_fds, std::map<int, Client> &clients, size_t i, Server &server)
{
	std::cout << "\n###################################################" << std::endl;
	std::cout << "################## ADDING CLIENT ##################" << std::endl;
	std::cout << "###################################################\n"
			  << std::endl;
	std::cout << "\033[32m New connection! \033[0m" << std::endl;

	int client_fd = accept(poll_fds[i].fd, NULL, NULL);

	if (client_fd < 0)
	{
		if (errno == EMFILE || errno == ENFILE)
			std::cerr << "FD limit reached" << std::endl;
		else if (errno == ECONNABORTED || errno == EINTR)
			return;
		else
			perror("accept failed");
		return;
	}
	else
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
	if (clients[poll_fds[i].fd].get_status_code() < 300)
		clients[poll_fds[i].fd].processRequest();
	clients[poll_fds[i].fd].set_create_response();
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

void Service::poll_service()
{
	std::vector<struct pollfd> poll_fds;
	std::vector<struct pollfd> server_fds;
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
				add_client_to_polls(poll_fds, server_fds, clients, i, servers[server_index]);
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

void Service::handle_connection(std::vector<struct pollfd> &poll_fds, const size_t &i)
{
	Client &client = clients[poll_fds[i].fd];
	if (client.should_keep_alive() == true)
	{
		std::string save_buffer = "";
		// Keep connection open for next request
		poll_fds[i].events = POLLIN;
		if (client.leftover_chunk() == true)
		{
			std::cout << "there is data left: " << client._request._request_data << std::endl;
			save_buffer = client._request._request_data;
		}
		client.refresh_client();
		client._request._request_data = save_buffer;
		std::cout << "[Handle Conn] Connection kept alive for fd: " << poll_fds[i].fd << std::endl;
	}
	else
	{
		// Close connection
		std::cout << "[Handle Conn] Closing connection (sent Connection: close)" << std::endl;
		int fd = poll_fds[i].fd;

		if (close(fd))
		{
			std::cout << "[Handle Conn] Error closing client fd: " << fd << std::endl;
			return;
		}
		poll_fds.erase(poll_fds.begin() + i);
		clients.erase(fd);
		std::cout << "\033[32m [Handle Conn] Client " << fd << " disconnected. Total clients (with server): " << (poll_fds.size()) << "\033[0m" << std::endl;
	}
}

void Service::handle_disconnection(std::vector<struct pollfd> &poll_fds, const size_t &i)
{
	int fd = poll_fds[i].fd;
	std::cout << "[Handle Disconn] Handling disconnection for fd: " << fd << std::endl;

	close(fd);							  // Close file descriptor
	poll_fds.erase(poll_fds.begin() + i); // Remove from poll
	clients.erase(fd);					  // Remove from map
	std::cout << "[Handle Disconn] Handling disconnection (error/hangup) for fd: " << fd << std::endl;
}
// TODO:  merge handle connection and handle disconnection
// TODO: double check function naming format (lowerCamelCase or underscore)
