#include "server.hpp"

Server::Server():_name(""), _port(-1), _host(""), _root(""), _index(), _error_page(""), _client_max_body_size(-1), _CGI() {}

Server::Server(const Server& other) 
{
	this->_name = other._name;
	this->_port = other._port;
	this->_host = other._host;
	this->_root = other._root;
	this->_index = other._index;
	this->_error_page = other._error_page;
	this->_client_max_body_size = other._client_max_body_size;
	this->_CGI = other._CGI;
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
		this->_CGI = other._CGI;
	}
	return (*this);
}

Server::~Server() {}

void	Server::set_name(std::string& name) { this->_name = name; }

std::string	Server::get_name() { return (this->_name); }

void	Server::set_port(int port) { this->_port = port; }

int	Server::get_port() { return (this->_port); }

void	Server::set_host(std::string& host) { this->_host = host; }

std::string	Server::get_host() { return (this->_host); }

void	Server::set_root(std::string& root) { this->_root = root; }

std::string	Server::get_root() { return (this->_root); }

void	Server::set_index(std::vector <std::string> index) { this->_index = index; }

std::vector <std::string>	Server::get_index() { return (this->_index); }

void	Server::set_error_page(std::string& page) { this->_error_page = page; }

std::string	Server::get_error_page() { return (this->_error_page); }

void	Server::set_client_max_body_size(int max) { this->_client_max_body_size = max; }

int	Server::get_client_max_body_size() { return (this->_client_max_body_size); }

void	Server::insert_CGI(std::string& key, std::map<std::string, std::string> value) { this->_CGI.insert(std::make_pair(key, value)); }

std::map <std::string, std::map<std::string, std::string> >	Server::get_CGI() { return (this->_CGI); }