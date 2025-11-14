#include "response.hpp"

Response::Response(): _response_data(""), _header(""), _body(""), _content_length(0), _allowedMethods() {}

Response::Response(const Response& other) 
{
	_response_data = other._response_data;
	_header = other._header;
	_body = other._body;
	_content_length = other._content_length;
	_allowedMethods = other._allowedMethods;
}

Response& Response::operator=(const Response& other)
{
	if (this != &other)
	{
		_response_data = other._response_data;
		_header = other._header;
		_body = other._body;
		_content_length = other._content_length;
		_allowedMethods = other._allowedMethods;
	}
	return (*this);
}

Response::~Response() {}

