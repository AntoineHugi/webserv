#include "response.hpp"

Response::Response(): _response_data(""), _header(""), _body(""), _content_length(0) {}

Response::Response(const Response& other) 
{
	_response_data = other._response_data;
	_header = other._header;
	_body = other._body;
	_content_length = other._content_length;
}

Response& Response::operator=(const Response& other)
{
	if (this != &other)
	{
		_response_data = other._response_data;
		_header = other._header;
		_body = other._body;
		_content_length = other._content_length;
	}
	return (*this);
}

Response::~Response() {}

