#include "client.hpp"

void Client::set_flags_error()
{
	_flags._should_keep_alive = false;
}

void Client::set_flags()
{
	if (_request.get_version() == "HTTP/1.1")
	{
		_flags._should_keep_alive = true;
		if (_request._header_kv["connection"] == "close")
			_flags._should_keep_alive = false;
	}
	else if (_request.get_version() == "HTTP/1.0")
	{
		_flags._should_keep_alive = false;
		if (_request._header_kv["connection"] == "keep-alive")
			_flags._should_keep_alive = true;
	}

	if (_request._header_kv["transfer-encoding"] == "chunked")
		_flags._body_chunked = true;
	else
		_flags._body_chunked = false;
}

void Client::refresh_client()
{
	if (DEBUG)
		std::cout << ">>>   CLIENT REFRESH <<<" << std::endl;
	_request.flush_request_data();
	_response.flush_response_data();
	_state = READING_HEADERS;
	_flags._should_keep_alive = false;
	_flags._body_chunked = false;
	_status_code = 200;
}

void	Client::create_default_error()
{
	std::string msg = "<!DOCTYPE html><html><head><title>404 Not Found</title></head><body>  <h1>404 Not Found</h1>  <p>The requested resource was not found on this server.</p></body></html>";
	_response.set_content_type("text/html");
	_response.set_body(msg);
}
