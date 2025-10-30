#include "request.hpp"


Request::Request(): _request_data("") {}

Request::Request(const Request& other)
{
	(void)other;
}

Request& Request::operator=(const Request& other)
{
	(void)other;
	return (*this);
}

Request::~Request() {}
