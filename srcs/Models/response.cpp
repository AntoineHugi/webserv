#include "response.hpp"

Response::Response():
	_header(""),
	_body(""),
	_content_length(0),
	_bytes_sent(0),
	_response_data("") {}

Response::Response(const Response& other) :
	_header (other._header),
	_body (other._body),
	_content_length (other._content_length),
	_bytes_sent (other._bytes_sent),
	_response_data (other._response_data) {}

Response& Response::operator=(const Response& other)
{
	if (this != &other)
	{
		_header = other._header;
		_body = other._body;
		_content_length = other._content_length;
		_bytes_sent = other._bytes_sent;
		_response_data = other._response_data;
	}
	return (*this);
}

Response::~Response() {}


std::string Response::format_response(int status_code, bool should_keep_alive, std::string version)
{
	std::string response;
	std::string reason_phrase;
	std::ostringstream ss;
	// client._status_code = 200;
	ss << status_code;

	// response += client._request._header_kv["Version"] + ss.str() + " ";
	response += version + " " + ss.str() + " ";
	reason_phrase = get_reason_phrase(status_code);
	if (reason_phrase == "Internal Server Error\r\n")
	{
		response += "Content-Length: 0\r\n";
		response += "Connection: close\r\n";
		return response;
	}
	response += reason_phrase;
	ss.str("");
	ss.clear();
	ss << this->_content_length;
	response += "Content-Length: " + ss.str() + "\r\n";
	response += "Content-Type: application/json\r\n"; // TODO: to be dynamic based on body
	response += "Server: webserv42\r\n";

	std::time_t now = std::time(0);
	std::tm* gmt_time = std::gmtime(&now);
  char date_buf[100];
  strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S GMT", gmt_time);
  response += "Date: ";
  response += date_buf;
  response += "\r\n";

	if (should_keep_alive)
			response += "Connection: keep-alive\r\n";
	else
			response += "Connection: close\r\n";
	response += "\r\n";

	if (this->_content_length != 0)
		response += this->_body;

	return response;
}
