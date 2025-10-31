#include "request.hpp"
#include <sstream>
#include <string>
#include <cstdlib>

Request::Request(): _request_data(""), _header(""), _body(""), _content_length(0), _header_parsed(false), _work_request(false), _status_code(0), _inORout(false), _should_keep_alive(false), _header_kv() {}

Request::Request(const Request& other)
{
	_request_data = other._request_data;
	_header = other._header;
	_body = other._body;
	_content_length = other._content_length;
	_header_parsed = other._header_parsed;
	_work_request = other._work_request;
	_status_code = other._status_code;
	_inORout = other._inORout;
	_header_kv = other._header_kv;
}

Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
			_request_data = other._request_data;
			_header = other._header;
			_body = other._body;
			_content_length = other._content_length;
			_header_parsed = other._header_parsed;
			_work_request = other._work_request;
			_status_code = other._status_code;
			_inORout = other._inORout;
			_header_kv = other._header_kv;
	}
	return (*this);
}

Request::~Request() {}


void parse_header(Request &req)
{
	std::cout << "\033[34mParsing header...\n\033[0m" << std::endl;
	std::cout << req._request_data << std::endl;
	std::string line;
	std::istringstream stream(req._header);

	std::getline(stream, line);
	size_t first_space = line.find(" ");
	size_t second_space = line.find(" ", first_space + 1);

	if (first_space != std::string::npos && second_space != std::string::npos) {
			req._header_kv["Method"] = line.substr(0, first_space);
			req._header_kv["URI"] = line.substr(first_space + 1, second_space - first_space - 1);
			req._header_kv["Version"] = line.substr(second_space + 1);
			if (!req._header_kv["Version"].empty() && req._header_kv["Version"].substr(req._header_kv["Version"].size()-1) == "\r")
				req._header_kv["Version"].erase(req._header_kv["Version"].size() - 1);
		}

	while (std::getline(stream, line)) {
		size_t colon_pos = line.find(":");
		if (colon_pos != std::string::npos) {
				std::string key = line.substr(0, colon_pos);
				key.erase(key.find_last_not_of(" ") + 1);
				std::string value = line.substr(colon_pos + 1, line.size() - 1);
				value.erase(0, value.find_first_not_of(" "));
				if (!value.empty() && value.substr(value.size()-1) == "\r")
					value.erase(value.size() - 1);
				req._header_kv[key] = value;
		}
	}
	req._header_parsed = true;
	req._content_length = req._header_kv["Content-Length"].empty() ? 0 : std::atoi(req._header_kv["Content-Length"].c_str());

	if (req._header_kv["Version"] == "HTTP/1.1") // HTTP/1.1 has keep-alive by default
	{
		req._should_keep_alive = true;
		if (req._header_kv["Connection"] == "close")
			req._should_keep_alive = false;
	}
	else if (req._header_kv["Version"] == "HTTP/1.0") 	// HTTP/1.0 requires explicit keep-alive
	{
		req._should_keep_alive = false;
		if (req._header_kv["Connection"] == "keep-alive")
			req._should_keep_alive = true;
	}

	std::map<std::string, std::string>::iterator it;
	for (it = req._header_kv.begin(); it != req._header_kv.end(); ++it) {
			std::cout << "'" << it->first << "' : '" << it->second << "'" << std::endl;
	}
}

void parse_body(Request &req)
{
	req._work_request = true;

}

void process_request(Request &req)
{
	std::cout << "\033[34mProcessing request of type: "<< req._header_kv["Method"] <<"\n\033[0m" << std::endl;
	req._status_code = 200;

	// Assumes the request was fully parsed
	// We then check what the request wants (POST, GET, DELETE etc) and process accordingly
	// After we have processed the request, we set the status code
	// we return to the poll loop where we will send the response based on the status code
}

void flush_request_data(Request &req)
{
	req._request_data.clear();
	req._header.clear();
	req._body.clear();
	req._content_length = 0;
	req._header_parsed = false;
	req._work_request = false;
	req._status_code = 0;
	req._inORout = false;
	req._should_keep_alive = false;
	req._header_kv.clear();
}

std::string format_response(Request &req)
{
	std::string response;
	std::ostringstream ss;
	ss << req._status_code;
	response += "HTTP/1.1 " + ss.str() + " ";

	switch (req._status_code) {
		case 200:
			response += "OK\r\n";
			break;
		case 400:
			response += "Bad Request\r\n";
			break;
		case 404:
			response += "Not Found\r\n";
			break;
		case 431:
			response += "Request Header Fields Too Large\r\n";
			break;
		default:
			response += "Internal Server Error\r\n";
			break;
	}

	response += "Content-Length: 0\r\n";
	if (req._should_keep_alive)
			response += "Connection: keep-alive\r\n";
	else
			response += "Connection: close\r\n";
	response += "\r\n";

	return response;
}
