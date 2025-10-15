#include "server.hpp"

Server::Server():_name(""), _port(""), _host(""), _root(""), _index(""), _error_page("") {}

Server::Server(const Server& other) 
{
	(void)other;
}

Server& Server::operator=(const Server& other)
{
	(void)other;
	return (*this);
}

Server::~Server() {}
