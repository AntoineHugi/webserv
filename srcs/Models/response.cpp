#include "response.hpp"

Response::Response() : _header(""),
					   _content_length(0),
					   _bytes_sent(0),
					   _response_data(""),
					   _body(""),
					   _allowedMethods() {}

Response::Response(const Response &other) : _header(other._header),
											_content_length(other._content_length),
											_bytes_sent(other._bytes_sent),
											_response_data(other._response_data),
											_body(other._body),
											_allowedMethods(other._allowedMethods) {}

Response &Response::operator=(const Response &other)
{
	if (this != &other)
	{
		_header = other._header;
		_content_length = other._content_length;
		_bytes_sent = other._bytes_sent;
		_response_data = other._response_data;
		_body = other._body;
		_allowedMethods = other._allowedMethods;
	}
	return (*this);
}

Response::~Response() {}

void Response::flush_response_data()
{
	_response_data.clear();
	_header.clear();
	_body.clear();
	_content_length = 0;
	_bytes_sent = 0;
}

std::string Response::get_reason_phrase(int status_code)
{
	switch (status_code)
	{
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 204:
		return "No Content";
	case 301:
		return "Moved Permanently";
	case 302:
		return "Found";
	case 400:
		return "Bad Request";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 413:
		return "Payload Too Large";
	case 431:
		return "Request Header Fields Too Large";
	case 500:
		return "Internal Server Error";
	case 505:
		return "HTTP Version Not Supported";
	}
	return "Internal Server Error";
}

std::string Response::format_response(int status_code, bool should_keep_alive, std::string version)
{
	std::string response;
	std::string reason_phrase;
	std::ostringstream ss;

	ss << status_code;
	if (version.empty() || (version != "HTTP/1.1" && version != "HTTP/1.0"))
		version = "HTTP/1.1";
	response += version + " " + ss.str() + " ";

	reason_phrase = get_reason_phrase(status_code);
	response += reason_phrase + "\r\n";
	if (!_body.empty())
		response += "Content-Type: " + _content_type + "\r\n";
	if (!get_location().empty())
		response += "Location: " + get_location() + "\r\n";
	if (status_code == 405)
	{
		response += "Allowed-Methods: ";
		for (size_t i = 0; i < get_allowed_methods().size(); ++i)
			response += get_allowed_methods()[i];
		response += "\r\n";
	}

	response += "Server: webserv42\r\n";
	std::time_t now = std::time(0);
	std::tm *gmt_time = std::gmtime(&now);
	char date_buf[100];
	strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S GMT", gmt_time);
	response += "Date: ";
	response += date_buf;
	response += "\r\n";

	if (reason_phrase == "Internal Server Error")
	{
		response += "Content-Length: 0\r\n";
		response += "Connection: close\r\n";
		return response;
	}
	if (should_keep_alive)
		response += "Connection: keep-alive\r\n";
	else
		response += "Connection: close\r\n";

	ss.str("");
	ss << _body.size();
	response += "Content-Length: " + ss.str() + "\r\n";
	response += "\r\n";

	if (_body.size() != 0)
		response += _body;
	return response;
}
