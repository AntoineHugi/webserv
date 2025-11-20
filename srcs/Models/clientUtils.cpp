#include "client.hpp"

void Client::set_flags_error()
{
	_flags._should_keep_alive = false;
}

void Client::set_flags()
{
	if (_request._version == "HTTP/1.1") // HTTP/1.1 has keep-alive by default
	{
		_flags._should_keep_alive = true;
		if (_request._header_kv["connection"] == "close")
			_flags._should_keep_alive = false;
	}
	else if (_request._version == "HTTP/1.0") // HTTP/1.0 requires explicit keep-alive
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
	std::cout << ">>>   CLIENT REFRESH <<<" << std::endl;
	_request.flush_request_data();
	_response.flush_response_data();
	_state = READING_HEADERS;
	_flags._should_keep_alive = false;
	_flags._body_chunked = false;
	//_flags._leftover_chunk = false;
	_status_code = 200;
}
