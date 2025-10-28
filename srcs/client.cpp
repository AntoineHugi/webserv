#include "client.hpp"


Client::Client(): _request_data("") {}

Client::Client(const Client& other)
{
	(void)other;
}

Client& Client::operator=(const Client& other)
{
	(void)other;
	return (*this);
}

Client::~Client() {}
