#include "service.hpp"

void Service::add_client_to_polls(std::map<int, Client> &clients, int fd, Server &server)
{
	print_header("ADDING CLIENTS TO POLL");
	print_green("New connection!", DEBUG);

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);

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
		print_cyan("Client connected from IP: " + client_ip , DEBUG);
		this->add_poll_to_vectors(client_fd, POLLIN | POLLOUT, "");
		clients.insert(std::pair<int, Client>(client_fd, Client(client_fd, server, client_ip)));
	}
}

void Service::set_polls()
{
	std::vector<struct pollfd> poll_fds;
	std::vector<struct pollfd> server_fds;
	std::vector<struct pollfd> cgi_fds;
	std::vector<struct pollfd> files_fds;


	this->fds.insert(std::pair<std::string, std::vector<struct pollfd> >("poll_fds", poll_fds));
	this->fds.insert(std::pair<std::string, std::vector<struct pollfd> >("server_fds", server_fds));
	this->fds.insert(std::pair<std::string, std::vector<struct pollfd> >("cgi_fds", cgi_fds));
	this->fds.insert(std::pair<std::string, std::vector<struct pollfd> >("files_fds", files_fds));

	for (size_t i = 0; i < this->servers.size(); i++)
		add_poll_to_vectors(servers[i].get_sock(), POLLIN, "server_fds");
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
	close(fd);
	print_yellow(" // Removing fds //", DEBUG);

	
	int index = find_fd_index_in_vector(fd, this->fds["poll_fds"]);
	print_yellow(" Index is: " + convert_to_string(index), DEBUG);
	if (index != -1)
		this->fds["poll_fds"].erase(this->fds["poll_fds"].begin() + index);

	index = find_fd_index_in_vector(fd, this->fds["cgi_fds"]);
	print_yellow(" Index is: " + convert_to_string(index), DEBUG);
	if (index != -1)
		this->fds["cgi_fds"].erase(this->fds["cgi_fds"].begin() + index);

	index = find_fd_index_in_vector(fd, this->fds["files_fds"]);
	print_yellow(" Index is: " + convert_to_string(index), DEBUG);
	if (index != -1)
		this->fds["files_fds"].erase(this->fds["files_fds"].begin() + index);

	clients.erase(fd);
	cgi_processes.erase(fd);
	files_fds.erase(fd);
}

void Service::add_poll_to_vectors(int fd, int events, std::string additional_poll)
{
	struct pollfd ptc;
	ptc.fd = fd;
	ptc.events = events;
	ptc.revents = 0;
	this->fds["poll_fds"].push_back(ptc);
	if (additional_poll == "cgi_fds")
		this->fds["cgi_fds"].push_back(ptc);
	if (additional_poll == "server_fds")
		this->fds["server_fds"].push_back(ptc);
	if (additional_poll == "files_fds")
		this->fds["files_fds"].push_back(ptc);
	return;
}
