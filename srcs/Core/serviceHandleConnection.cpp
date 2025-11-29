#include "service.hpp"

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
			if (DEBUG)
				std::cout << "there is data left: " << client._request._request_data << std::endl;
			save_buffer = client._request._request_data;
		}
		client.refresh_client();
		client._request._request_data = save_buffer;
		if (DEBUG)
			std::cout << "[Handle Conn] Connection kept alive for fd: " << poll_fds[i].fd << std::endl;
	}
	else
	{
		// Close connection
		if (DEBUG)
			std::cout << "[Handle Conn] Closing connection (sent Connection: close)" << std::endl;
		int fd = poll_fds[i].fd;

		if (close(fd))
		{
			std::cout << "[Handle Conn] Error closing client fd: " << fd << std::endl;
			return;
		}
		poll_fds.erase(poll_fds.begin() + i);
		clients.erase(fd);
		if (DEBUG)
			std::cout << "\033[32m [Handle Conn] Client " << fd << " disconnected. Total clients (with server): " << (poll_fds.size()) << "\033[0m" << std::endl;
	}
}

void Service::handle_disconnection(std::vector<struct pollfd> &poll_fds, const size_t &i)
{
	int fd = poll_fds[i].fd;
	if (DEBUG)
		std::cout << "[Handle Disconn] Handling disconnection for fd: " << fd << std::endl;

	close(fd);			      // Close file descriptor
	poll_fds.erase(poll_fds.begin() + i); // Remove from poll
	clients.erase(fd);		      // Remove from map
	if (DEBUG)
		std::cout << "[Handle Disconn] Handling disconnection (error/hangup) for fd: " << fd << std::endl;
}

void Service::handle_disconnection_by_fd(int fd)
{
	for (size_t i = 0; i < fds["poll_fds"].size(); ++i)
	{
		if (fds["poll_fds"][i].fd == fd)
		{
			handle_disconnection(fds["poll_fds"], i);
			return;
		}
	}
}
