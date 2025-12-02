#include "response.hpp"

Response::Response() :
	_header(""),
	_content_length(0),
	_bytes_sent(0),
	_response_buffer(""),
	_response_data(""),
	_body(""),
	_allowedMethods(),
	_content_type(""),
	_location("")
{}

Response::Response(const Response &other) :
	_header(other._header),
	_content_length(other._content_length),
	_bytes_sent(other._bytes_sent),
	_response_data(other._response_data),
	_body(other._body),
	_allowedMethods(other._allowedMethods),
	_content_type(other._content_type) ,
	_location(other._location)
{}

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
		_content_type = other._content_type;
		_location = other._location;
	}
	return (*this);
}

Response::~Response() {}

/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/

void Response::flush_response_data()
{
	_header.clear();
	_content_length = 0;
	_bytes_sent = 0;
	_response_buffer.clear();
	_response_data.clear();
	_body.clear();
	_allowedMethods.clear();
	_content_type.clear();
	_location.clear();
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

/* Helper: Parse CGI headers into a map (lowercase keys for case-insensitive comparison) */
std::map<std::string, std::string> Response::parse_cgi_headers(const std::string &header_text)
{
	std::map<std::string, std::string> headers;
	std::string remaining = header_text;

	while (remaining.find("\n") != std::string::npos)
	{
		size_t pos = remaining.find("\n");
		std::string line = remaining.substr(0, pos);
		remaining = remaining.substr(pos + 1);

		size_t colon = line.find(":");
		if (colon != std::string::npos)
		{
			std::string name = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			size_t start = value.find_first_not_of(" \t\r");
			if (start != std::string::npos)
				value = value.substr(start);
			size_t end = value.find_last_not_of(" \t\r");
			if (end != std::string::npos)
				value = value.substr(0, end + 1);
			for (size_t i = 0; i < name.length(); i++)
				name[i] = std::tolower(name[i]);
			headers[name] = value;
		}
	}

	if (!remaining.empty())
	{
		size_t colon = remaining.find(":");
		if (colon != std::string::npos)
		{
			std::string name = remaining.substr(0, colon);
			std::string value = remaining.substr(colon + 1);
			size_t start = value.find_first_not_of(" \t\r");
			if (start != std::string::npos)
				value = value.substr(start);
			size_t end = value.find_last_not_of(" \t\r");
			if (end != std::string::npos)
				value = value.substr(0, end + 1);
			for (size_t i = 0; i < name.length(); i++)
				name[i] = std::tolower(name[i]);
			headers[name] = value;
		}
	}

	return headers;
}

/* Helper: Capitalize header name for output (e.g., "content-type" -> "Content-Type") */
std::string Response::capitalize_header(const std::string &name)
{
	std::string result = name;
	bool capitalize_next = true;

	for (size_t i = 0; i < result.length(); i++)
	{
		if (capitalize_next && result[i] >= 'a' && result[i] <= 'z')
			result[i] = std::toupper(result[i]);
		else if (!capitalize_next && result[i] >= 'A' && result[i] <= 'Z')
			result[i] = std::tolower(result[i]);

		capitalize_next = (result[i] == '-');
	}

	return result;
}

/*

  | Header         | Priority      | Notes                            |
  |----------------|---------------|----------------------------------|
  | Status         | CGI wins      | Overrides server status code     |
  | Server         | Server ALWAYS | Never from CGI                   |
  | Date           | Server ALWAYS | Never from CGI                   |
  | Connection     | Server ALWAYS | Server manages keep-alive        |
  | Content-Type   | CGI wins      | Falls back to server default     |
  | Content-Length | CGI wins      | Server calculates if missing     |
  | Location       | CGI/Server    | From CGI if present, else server |
  | Set-Cookie     | CGI wins      | Passed through                   |
  | Custom headers | CGI wins      | Any X-* or application headers   |

*/

void Response::format_response(int status_code, bool should_keep_alive, std::string version)
{
	std::ostringstream ss;

	std::map<std::string, std::string> cgi_headers;
	int final_status = status_code;

	if (!get_header().empty())
	{
		cgi_headers = parse_cgi_headers(get_header());
		if (cgi_headers.find("status") != cgi_headers.end())
		{
			std::istringstream iss(cgi_headers["status"]);
			iss >> final_status;
			cgi_headers.erase("status");
		}
	}

	if (version.empty() || (version != "HTTP/1.1" && version != "HTTP/1.0"))
		version = "HTTP/1.1";

	ss << final_status;
	_response_data += version + " " + ss.str() + " ";
	_response_data += get_reason_phrase(final_status) + "\r\n";
	_response_data += "Server: webserv42\r\n";

	std::time_t now = std::time(0);
	std::tm *gmt_time = std::gmtime(&now);
	char date_buf[100];
	strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S GMT", gmt_time);
	_response_data += "Date: ";
	_response_data += date_buf;
	_response_data += "\r\n";

	if (final_status == 500)
	{
		_response_data += "Content-Length: 0\r\n";
		_response_data += "Connection: close\r\n";
		_response_data += "\r\n";
		std::cout << "status code sent : " << final_status << std::endl;
		return;
	}

	if (status_code == 301 || status_code == 302)
	{
		if (cgi_headers.find("location") != cgi_headers.end())
		{
			_response_data += "Location: " + cgi_headers["location"] + "\r\n";
			cgi_headers.erase("location");
		}
		else
			_response_data += "Location: " + get_location() + "\r\n";
	}

	if (status_code == 405)
	{
		_response_data += "Allowed-Methods: ";
		for (size_t i = 0; i < get_allowed_methods().size(); ++i)
			_response_data += get_allowed_methods()[i];
		_response_data += "\r\n";
	}

	for (std::map<std::string, std::string>::iterator it = cgi_headers.begin();
	     it != cgi_headers.end(); ++it)
	{
		std::string header_name = it->first;
		if (header_name == "server" || header_name == "date" || header_name == "connection")
			continue;
		_response_data += capitalize_header(header_name) + ": " + it->second + "\r\n";
	}

	/* Add required headers if CGI didn't provide them */
	if (cgi_headers.find("content-type") == cgi_headers.end() && !_body.empty())
		_response_data += "Content-Type: " + _content_type + "\r\n";

	if (cgi_headers.find("content-length") == cgi_headers.end())
	{
		ss.str("");
		ss << _body.size();
		_response_data += "Content-Length: " + ss.str() + "\r\n";
	}

	if (cgi_headers.find("connection") == cgi_headers.end())
	{
		if (should_keep_alive)
			_response_data += "Connection: keep-alive\r\n";
		else
			_response_data += "Connection: close\r\n";
	}

	_response_data += "\r\n";
	if (!_body.empty())
		_response_data += _body;

	print_blue("status code sent : " + convert_to_string(final_status), DEBUG);
	return;
}
