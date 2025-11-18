#include "response.hpp"

Response::Response():
	_header(""),
	_content_length(0),
	_bytes_sent(0),
	_response_data(""),
	_body(""),
	_allowedMethods() {}

Response::Response(const Response& other) :
	_header (other._header),
	_content_length (other._content_length),
	_bytes_sent (other._bytes_sent),
	_response_data (other._response_data),
	_body (other._body),
	_allowedMethods(other._allowedMethods) {}

Response& Response::operator=(const Response& other)
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


std::string Response::format_response(int status_code, bool should_keep_alive, std::string version, std::string body)
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

	ss.str("");
	ss << body.size();
	response += "Content-Length: " + ss.str() + "\r\n";
	response += "\r\n"; // close headers

	// Append body if present
	if (body.size() != 0)
		response += body;


	return response;
}
