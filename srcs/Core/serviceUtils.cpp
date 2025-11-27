#include "service.hpp"

void add_client_to_polls(std::vector<struct pollfd> &poll_fds, std::map<int, Client> &clients, int fd, Server& server)
{
	std::cout << "\n###################################################" << std::endl;
	std::cout << "################## ADDING CLIENT ##################" << std::endl;
	std::cout << "###################################################\n"
			  << std::endl;
	std::cout << "\033[32m New connection! \033[0m" << std::endl;

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);

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
		char ip_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
		std::string client_ip(ip_str);
		std::cout << "Client connected from IP: " << client_ip << std::endl;

		struct pollfd client_pfd;
		client_pfd.fd = client_fd;
		client_pfd.events = POLLIN | POLLOUT; // Wait for data from client
		client_pfd.revents = 0;
		poll_fds.push_back(client_pfd);
		clients.insert(std::pair<int, Client>(client_fd, Client(client_fd, server, client_ip)));
	}
}

void Service::set_polls()
{
	std::vector<struct pollfd> poll_fds;
	std::vector<struct pollfd> server_fds;
	std::vector<struct pollfd> cgi_fds;

	this->fds.insert(std::pair< std::string, std::vector<struct pollfd> >("poll_fds", poll_fds));
	this->fds.insert(std::pair< std::string, std::vector<struct pollfd> >("server_fds", server_fds));
	this->fds.insert(std::pair< std::string, std::vector<struct pollfd> >("cgi_fds", cgi_fds));

	for (size_t i = 0; i < this->servers.size(); i++)
	{
		struct pollfd pfd;
		pfd.fd = servers[i].get_sock();
		pfd.events = POLLIN;
		this->fds["poll_fds"].push_back(pfd);
		this->fds["server_fds"].push_back(pfd);
	}
}

int Service::server_fd_for_new_client(int fd, std::vector<struct pollfd> &fds_vector)
{
	for (size_t i = 0; i < fds_vector.size(); i++)
	{
		if (fds_vector[i].fd == fd)
			return (i);
	}
	return (-1);
}
int Service::cgi_fd_for_cgi(int fd, std::vector<struct pollfd> &fds_vector)
{
	for (size_t i = 0; i < fds_vector.size(); i++)
	{
		if (fds_vector[i].fd == fd)
			return (fd);
	}
	return (-1);
}

int find_fd_index_in_vector(int fd, std::vector<struct pollfd> &fds_vector)
{
	for (size_t i = 0; i < fds_vector.size(); i++)
	{
		if (fds_vector[i].fd == fd)
			return (i);
	}
	return (-1);
}

void Service::remove_fd(int fd)
{
	// Closing fd
	close(fd);
	std::cout <<  "removing fds" << std::endl;
	// Removing from poll vectors
	int index = find_fd_index_in_vector(fd, this->fds["poll_fds"]);
	if (index != -1)
		this->fds["poll_fds"].erase(this->fds["poll_fds"].begin() + index);
	index = find_fd_index_in_vector(fd, this->fds["cgi_fds"]);
	if (index != -1)
		this->fds["cgi_fds"].erase(this->fds["cgi_fds"].begin() + index);

	// Removing from clients and cgi_processes maps
	clients.erase(fd);
	cgi_processes.erase(fd);
}
