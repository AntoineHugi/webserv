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

void add_client_to_polls(std::vector<struct pollfd> &poll_fds, std::map<int, Client> &clients, size_t i, Server& server)
{
	std::cout << "\n###################################################"<< std::endl;
	std::cout << "################## ADDING CLIENT #######################"<< std::endl;
	std::cout << "###################################################\n"<< std::endl;
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

int Service::service_reading(std::vector<struct pollfd> &poll_fds, int i)
{
	std::cout << "\n###################################################"<< std::endl;
	std::cout << "################## READING #######################"<< std::endl;
	std::cout << "###################################################\n"<< std::endl;

	if (this->clients[poll_fds[i].fd].handle_read())
	{
		// Check if it's an error (4xx/5xx) or clean close (recv == 0)
		int status = this->clients[poll_fds[i].fd].get_status_code();

		if (status >= 400)  // Actual error - send error response
		{
			std::cout << "[READING] Error detected (status " << status << "), sending error response" << std::endl;
			this->clients[poll_fds[i].fd].set_flags_error();
			this->clients[poll_fds[i].fd]._request._body = "";
			this->clients[poll_fds[i].fd]._response.set_response_data(this->clients[poll_fds[i].fd]._response.format_response
				(status,
				false,  // Always close on error
				this->clients[poll_fds[i].fd]._request._header_kv["version"],
				this->clients[poll_fds[i].fd]._request._body));

			this->clients[poll_fds[i].fd].set_send_response();
			poll_fds[i].events = POLLOUT;
		}
		else  // Clean close (recv == 0) - just disconnect
		{
			std::cout << "[READING] Client closed connection cleanly" << std::endl;
			this->handle_disconnection(poll_fds, i);
			return 1;  // Signal to continue (client already removed)
		}
		return (1);
	}
	else
	{
		std::cout << "Service ========>>  Client work request " << this->clients[poll_fds[i].fd].can_i_process_request() <<  std::endl;
		if (this->clients[poll_fds[i].fd].can_i_process_request() == true)
				poll_fds[i].events = POLLOUT;
	}
	return (0);
}

int Service::service_processing(std::vector<struct pollfd> &poll_fds, int i)
{
	std::cout << "\n###################################################"<< std::endl;
	std::cout << "################## PROCESSING #######################"<< std::endl;
	std::cout << "###################################################\n"<< std::endl;

	// need to figure out how to avoid delays, for example cgi
	// if (this->clients[poll_fds[i].fd]._server->validateRequest(clients[poll_fds[i].fd])) // TODO: change validateRequest ownlership to client accesing Server info
		//this->clients[poll_fds[i].fd]._server.processRequest(clients[poll_fds[i].fd]);
	//process_request(clients[poll_fds[i].fd]);
	//ex: this->clients[poll_fds[i].fd]._server.croupier(clients[poll_fds[i].fd]);
	//if cgi -> fork() and get response (this should be in the process_request function)
	// this->clients[poll_fds[i].fd].set_status_code(200);
	std::cout << "sttus: " << this->clients[poll_fds[i].fd].get_status_code() << std::endl;

	if (this->clients[poll_fds[i].fd]._response.get_response_data_full().size() > 8192)
		this->clients[poll_fds[i].fd].set_status_code(500);

	if (this->clients[poll_fds[i].fd].get_status_code() > 300)
	{
			this->clients[poll_fds[i].fd].set_flags_error();
			this->clients[poll_fds[i].fd]._request._body = "";
	}
	this->clients[poll_fds[i].fd]._response.set_response_data(this->clients[poll_fds[i].fd]._response.format_response
		(this->clients[poll_fds[i].fd].get_status_code(),
		this->clients[poll_fds[i].fd].should_keep_alive(),
		this->clients[poll_fds[i].fd]._request._header_kv["version"],
		this->clients[poll_fds[i].fd]._request._body));

	this->clients[poll_fds[i].fd].set_send_response();
	return (0);
}

int	Service::service_writing(std::vector<struct pollfd> &poll_fds, int i)
{
	std::cout << "\n###################################################"<< std::endl;
	std::cout << "################## WRITING #######################"<< std::endl;
	std::cout << "###################################################\n"<< std::endl;

	if(this->clients[poll_fds[i].fd].handle_write())
	{
		std::cout << "\n >> returning from inside writing - handle connection should happen outside << " << std::endl;
		// this->handle_connection(poll_fds, i);
		poll_fds[i].events = POLLIN;
		return (1);
	}
	else
	{
		std::cout << "Service ========>>  Client send response: " << this->clients[poll_fds[i].fd].can_i_send_response() <<  std::endl;
		if (!this->clients[poll_fds[i].fd].can_i_send_response())
		{
			std::cout << "[Writing] Finishing request, preparing client for next or closing" << std::endl;
			poll_fds[i].events = POLLIN;
			if (!this->clients[poll_fds[i].fd].can_i_close_connection())
				this->clients[poll_fds[i].fd].set_read_header();
			this->handle_connection(poll_fds, i);
		}
	}
	return (0);
}

void Service::poll_service()
{
	std::vector<struct pollfd> poll_fds;
	std::vector<struct pollfd> server_fds;
	std::vector<struct pollfd> cgi_fds;
	set_polls(poll_fds, server_fds, this->servers);

	while(g_shutdown == 0)
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
			if (poll_fds[i].revents == 0)
			{
				if (new_client || (!new_client && this->clients[poll_fds[i].fd].leftover_chunk() == false))
					continue;
				else if (this->clients[poll_fds[i].fd].is_inactive() && poll_fds[i].revents & POLLIN)
					this->handle_disconnection(poll_fds, i);
			}
			if (new_client)
			{
				add_client_to_polls(poll_fds, this->clients, i, this->servers[server_index]);
				std::cout << "\033[32m New client connected. Total clients: " << (poll_fds.size() - server_fds.size()) << "\033[0m" << std::endl;
			}
			else
			{
				Client& client = this->clients[poll_fds[i].fd];
				client.update_last_interaction();
				if (client.leftover_chunk() == false && poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
				{
					std::cout << "poll_fds[i].revents : " << poll_fds[i].revents << std::endl;
					this->handle_disconnection(poll_fds, i);
					continue;
				}
				if ((poll_fds[i].revents & POLLIN || client.leftover_chunk()) && (client.can_i_read_header() == true || client.can_i_read_body() == true))
					this->service_reading(poll_fds, i);
				if (i < (int)poll_fds.size() && (poll_fds[i].revents & POLLOUT) && client.can_i_process_request() == true)
					this->service_processing(poll_fds, i);
				if (i < (int)poll_fds.size() && (poll_fds[i].revents & POLLOUT) && client.can_i_send_response() == true)
					this->service_writing(poll_fds, i);
				if (i < (int)poll_fds.size() && poll_fds[i].revents & POLLIN && client.is_error() == true)
					this->handle_connection(poll_fds, i);

			}
		}
	}
	std::cout << "Shutting down gracefully..." << std::endl;
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		close(it->first);
	for (size_t i = 0; i < servers.size(); i++)
		close(servers[i].get_sock());
}

void	Service::handle_connection(std::vector<struct pollfd> &poll_fds, const size_t& i)
{
	Client& client = this->clients[poll_fds[i].fd];
	if (client.get_status_code() < 300)
	{
		if (client.should_keep_alive() == true)
		{
			std::string save_buffer = "";
			// Keep connection open for next request
			poll_fds[i].events = POLLIN;
			if (client.leftover_chunk() == true)
				save_buffer = client._request._request_data.substr(client._request._header.size() + client._request._body.size());
			client.refresh_client();
			client._request._request_data = save_buffer;
			std::cout << "[Handle Conn] Connection kept alive for fd: " << poll_fds[i].fd << std::endl;
		}
		else
		{
			// Close connection
			std::cout << "[Handle Conn] Closing connection (sent Connection: close)" << std::endl;
			int fd = poll_fds[i].fd;

			if(close(fd))
			{
				std::cout << "[Handle Conn] Error closing client fd: " << fd << std::endl;
				return;
			}
			poll_fds.erase(poll_fds.begin() + i);
			clients.erase(fd);
			std::cout << "\033[32m [Handle Conn] Client " << fd << " disconnected. Total clients (with server): " << (poll_fds.size()) << "\033[0m" << std::endl;
		}
	}
	else
	{
		std::cout << "==>> [Handle Conn]  Client will be closed" << std::endl;
		int fd = poll_fds[i].fd;
		int status = client.get_status_code();
		if(close(fd))
		{
			std::cout << "Error closing client fd: " << fd << std::endl;
			return;
		}
		poll_fds.erase(poll_fds.begin() + i);
		clients.erase(fd);
		std::cout << "[Handle Conn] Client " << fd << " disconnected, show status here: "<< status <<" ." << std::endl;
		return;
	}
}

void	Service::handle_disconnection(std::vector<struct pollfd> &poll_fds, const size_t& i)
{
	int fd = poll_fds[i].fd;
	std::cout << "[Handle Disconn] Handling disconnection for fd: " << fd << std::endl;

	close(fd);  // Close file descriptor
	poll_fds.erase(poll_fds.begin() + i);  // Remove from poll
	clients.erase(fd);  // Remove from map
	std::cout << "[Handle Disconn] Handling disconnection (error/hangup) for fd: " << fd << std::endl;
}
// TODO:  merge handle connection and handle disconnection
// TODO: double check function naming format (lowerCamelCase or underscore)
