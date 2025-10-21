#include "server.hpp"

Server::Server():_name(""), _port(-1), _host(""), _root(""), _index(), _error_page(""), _client_max_body_size(-1), _routes() {}

Server::Server(const Server& other) 
{
	this->_name = other._name;
	this->_port = other._port;
	this->_host = other._host;
	this->_root = other._root;
	this->_index = other._index;
	this->_error_page = other._error_page;
	this->_client_max_body_size = other._client_max_body_size;
	this->_routes = other._routes;
}

Server& Server::operator=(const Server& other)
{
	if (this != &other)
	{
		this->_name = other._name;
		this->_port = other._port;
		this->_host = other._host;
		this->_root = other._root;
		this->_index = other._index;
		this->_error_page = other._error_page;
		this->_client_max_body_size = other._client_max_body_size;
		this->_routes = other._routes;
	}
	return (*this);
}

Server::~Server() {}

void	Server::set_name(std::string& name) { this->_name = name; }
void	Server::set_port(int port) { this->_port = port; }
void	Server::set_host(std::string& host) { this->_host = host; }
void	Server::set_root(std::string& root) { this->_root = root; }
void	Server::set_index(std::vector <std::string> index) { this->_index = index; }
void	Server::set_error_page(std::string& page) { this->_error_page = page; }
void	Server::set_client_max_body_size(int max) { this->_client_max_body_size = max; }
void	Server::add_route(Route route) { this->_routes.push_back(route); }

std::string	Server::get_name() { return (this->_name); }
int	Server::get_port() { return (this->_port); }
std::string	Server::get_host() { return (this->_host); }
std::string	Server::get_root() { return (this->_root); }
std::vector <std::string>	Server::get_index() { return (this->_index); }
std::string	Server::get_error_page() { return (this->_error_page); }
int	Server::get_client_max_body_size() { return (this->_client_max_body_size); }
std::vector <Route>	Server::get_routes() { return (this->_routes); }