#include "request.hpp"


Request::Request(): _request_data(""), _header(""), _body(""), _content_length(0), _header_parsed(false), _work_request(false), _status_code(0), _inORout(0) {}

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
