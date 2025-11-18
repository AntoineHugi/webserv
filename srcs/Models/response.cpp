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

std::string Response::format_response(int status_code, bool should_keep_alive, std::string version)
{
	std::cout << "entered format response" << std::endl;
	std::string response;
	std::string reason_phrase;
	std::ostringstream ss;
	ss << status_code;

	response += version + " " + ss.str() + " ";
	reason_phrase = get_reason_phrase(status_code);
	if (reason_phrase == "Internal Server Error\r\n")
	{
		response += "Content-Length: 0\r\n";
		response += "Connection: close\r\n";
		return response;
	}
	response += reason_phrase;
	if (!_body.empty())
	{
		determine_content_type();
		response += "Content-Type: ";
		response += _content_type;
		response += "\r\n";
	}
	if (status_code == 405)
	{
		response += "Allowed-Methods: ";
		for (size_t i = 0; i < get_allowed_methods().size(); ++i)
			response += get_allowed_methods()[i];
		response += "\r\n";
	}
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
	ss << _body.size();
	response += "Content-Length: " + ss.str() + "\r\n";
	response += "\r\n"; // close headers

	// Append body if present
	if (_body.size() != 0)
		response += _body;
	std::cout << "end of format response: " << response << std::endl;
	return response;
}

void Response::determine_content_type()
{
	size_t dot = _request->_uri.find_last_of('.');
	if (dot == std::string::npos)
	{
		_content_type = "application/octet-stream";
		return;
	}
	std::string ext = _request->_uri.substr(dot + 1);
	for (size_t i = 0; i < ext.size(); i++)
		ext[i] = std::tolower(ext[i]);
	if (ext == "html" || ext == "htm") { _content_type = "text/html"; }
	else if (ext == "txt") { _content_type = "text/plain"; }
	else if (ext == "json") { _content_type = "application/json"; }
	else if (ext == "png") { _content_type = "image/png"; }
	else if (ext == "jpg" || ext == "jpeg") { _content_type = "image/jpeg"; }
	else if (ext == "webp") { _content_type = "image/webp"; }
	else if (ext == "gif") { _content_type = "image/gif"; }
	else if (ext == "pdf") { _content_type = "application/pdf"; }
	else if (ext == "css") { _content_type = "text/css"; }
	else if (ext == "js") { _content_type = "application/javascript"; }
	else if (ext == "py") { _content_type = "text/x-python"; }
	else { _content_type = "application/octet-stream"; }
	return;
}
