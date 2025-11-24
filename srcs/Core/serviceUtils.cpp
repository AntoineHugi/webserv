#include "service.hpp"

void add_client_to_polls(std::vector<struct pollfd> &poll_fds, std::map<int, Client> &clients, size_t i, Server& server)
{
	std::cout << "\n###################################################" << std::endl;
	std::cout << "################## ADDING CLIENT ##################" << std::endl;
	std::cout << "###################################################\n"
			  << std::endl;
	std::cout << "\033[32m New connection! \033[0m" << std::endl;

	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(poll_fds[i].fd, (struct sockaddr*)&client_addr, &client_len);

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
