#include "server.hpp"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h> // for socket handling
#include <fcntl.h> // for file descriptor handling
#include <cstring> // 
#include <netinet/in.h> // for htons


Server::Server():_name(""), _port(-1), _host(""), _root(""), _index(""), _error_page(""), _client_max_body_size(-1), _sock(-1), _clients() {}

Server::~Server() {
	
}

Server::Server(const Server& other) 
{
	this->_name = other._name;
	this->_port = other._port;
	this->_host = other._host;
	this->_root = other._root;
	this->_index = other._index;
	this->_error_page = other._error_page;
	this->_client_max_body_size = other._client_max_body_size;
	//TODO: Make deep copy of clients
}

Server& Server::operator=(const Server& other)
{
	(void)other;
	return (*this);
}



std::string	Server::get_name() { return (this->_name); }
int	Server::get_port() { return (this->_port); }
int	Server::get_sock() { return (this->_sock); }
std::string	Server::get_host() { return (this->_host); }
std::string	Server::get_root() { return (this->_root); }
std::string	Server::get_index() { return (this->_index); }
std::string	Server::get_error_page() { return (this->_error_page); }
int	Server::get_client_max_body_size() { return (this->_client_max_body_size); }

void	Server::set_name(std::string name) { this->_name = name; }
void	Server::set_port(int port) { this->_port = port; }
void	Server::set_sock(int sock) { this->_sock = sock; }
void	Server::set_host(std::string host) { this->_host = host; }
void	Server::set_root(std::string root) { this->_root = root; }
void	Server::set_index(std::string index) { this->_index = index; }
void	Server::set_error_page(std::string page) { this->_error_page = page; }
void	Server::set_client_max_body_size(int max) { this->_client_max_body_size = max; }


void Server::set_server(){

	this->set_sock(socket(AF_INET, SOCK_STREAM, 0));
	if (this->get_sock() < 0) {
		perror("Socket creation failed");
		return; // TODO: throw correct error
	}
	std::cout << "socket: " << this->get_sock() << std::endl;

	int optval = 1;
	int check = setsockopt(this->get_sock(), SOL_SOCKET , SO_REUSEADDR , &optval, sizeof(optval));
	if (check < 0) {
		perror("Socket option failed");
		return; // TODO: throw correct error
	}
	int flags = fcntl(this->get_sock(), F_GETFL); // Get current flags
	check = fcntl(this->get_sock(), F_SETFL, flags | O_NONBLOCK); // Set non-blocking flag
	// Interesting for CGI
	// fcntl(fd, F_SETFD, FD_CLOEXEC); // Set close-on-exec flag
	if (check < 0) {
		perror("Socket flags failed");
		return; // TODO: throw correct error
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET; // IPv4
	server_addr.sin_addr.s_addr = INADDR_ANY; // Any interface
	server_addr.sin_port = htons(8080); // Port 8080
    check = bind(this->get_sock(), (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (check < 0) {
		perror("Bind failed");
		return; // TODO: throw correct error
    }
	check = listen(this->get_sock(), 128);
	if (check < 0) {
		perror("Listen failed");
		return; // TODO: throw correct error
    }
	std::cout << "Server listening on port " << this->get_port() << std::endl;
}


