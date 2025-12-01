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
		/* Use timeout 0 if any client has leftover data to process */
		int POLL_TIMEOUT = CLIENT_TIMEOUT_MS;
		for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			if (it->second.leftover_chunk())
			{
				POLL_TIMEOUT = 0;
				break;
			}
		}
		int ret = poll(this->fds["poll_fds"].data(), this->fds["poll_fds"].size(), POLL_TIMEOUT);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			perror("poll failed");
			break;
		}
		if (ret == 0)
		{
			for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end();)
			{
				if (it->second.is_inactive() && !it->second.leftover_chunk() && !it->second.am_i_waiting_cgi())
				{
					int fd = it->first;
					++it;
					handle_disconnection_by_fd(fd);
				}
				else
					++it;
			}
		}
		for (int i = this->fds["poll_fds"].size() - 1; i >= 0; i--)
		{

			int server_fd_if_new_client = server_fd_for_new_client(this->fds["poll_fds"][i].fd, this->fds["server_fds"]);
			int cgi_fd_if_cgi = cgi_fd_for_cgi(this->fds["poll_fds"][i].fd, this->fds["cgi_fds"]);

			if (server_fd_if_new_client != -1)
			{
				if (this->fds["poll_fds"][i].revents != 0)
				{
					add_client_to_polls(this->clients, this->fds["poll_fds"][i].fd, this->servers[server_fd_if_new_client]);
					std::ostringstream ss;
					ss << (this->fds["poll_fds"].size() - this->fds["server_fds"].size());
					std::string msg = "New client connected. Total clients: " + ss.str();
					print_cyan(msg, DEBUG);
				}
			}
			else if (cgi_fd_if_cgi != -1)
				cgi_handler(i);
			else
			{
				Client &client = clients[this->fds["poll_fds"][i].fd];
				if (client.leftover_chunk() == false && this->fds["poll_fds"][i].revents & (POLLERR | POLLHUP | POLLNVAL))
				{
					if (DEBUG)
					{
						std::cout << "POLL event : " << this->fds["poll_fds"][i].revents << std::endl;
						std::cout << "this->fds['poll_fds'][i].revents : " << this->fds["poll_fds"][i].revents << std::endl;
					}
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
