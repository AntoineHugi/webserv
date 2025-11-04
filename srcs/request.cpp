#include "request.hpp"
#include <sstream>
#include <string>
#include <cstdlib>

Request::Request(): _request_data(""), _header(""), _body(""), _header_kv(), _content_length(0) {}

Request::Request(const Request& other)
{
	_request_data = other._request_data;
	_header = other._header;
	_body = other._body;
	_content_length = other._content_length;
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
			_header_kv = other._header_kv;
	}
	return (*this);
}

Request::~Request() {}

void Request::flush_request_data()
{
	this->_request_data.clear();
	this->_header.clear();
	this->_body.clear();
	this->_content_length = 0;
	this->_header_kv.clear();
}

// void Request::parse_header()
// {
// 	std::cout << "\033[34mParsing header...\n\033[0m" << std::endl;
// 	std::cout << this->_request_data << std::endl;
// 	std::string line;
// 	std::istringstream stream(this->_header);

// 	std::getline(stream, line);
// 	size_t first_space = line.find(" ");
// 	size_t second_space = line.find(" ", first_space + 1);

// 	if (first_space != std::string::npos && second_space != std::string::npos) {
// 			this->_header_kv["Method"] = line.substr(0, first_space);
// 			this->_header_kv["URI"] = line.substr(first_space + 1, second_space - first_space - 1);
// 			this->_header_kv["Version"] = line.substr(second_space + 1);
// 			if (!this->_header_kv["Version"].empty() && this->_header_kv["Version"].substr(this->_header_kv["Version"].size()-1) == "\r")
// 				this->_header_kv["Version"].erase(this->_header_kv["Version"].size() - 1);
// 		}

// 	while (std::getline(stream, line)) {
// 		size_t colon_pos = line.find(":");
// 		if (colon_pos != std::string::npos) {
// 				std::string key = line.substr(0, colon_pos);
// 				key.erase(key.find_last_not_of(" ") + 1);
// 				std::string value = line.substr(colon_pos + 1, line.size() - 1);
// 				value.erase(0, value.find_first_not_of(" "));
// 				if (!value.empty() && value.substr(value.size()-1) == "\r")
// 					value.erase(value.size() - 1);
// 				this->_header_kv[key] = value;
// 		}
// 	}
// 	this->_header_parsed = true;
// 	this->_content_length = this->_header_kv["Content-Length"].empty() ? 0 : std::atoi(this->_header_kv["Content-Length"].c_str());

// 	if (this->_header_kv["Version"] == "HTTP/1.1") // HTTP/1.1 has keep-alive by default
// 	{
// 		this->_should_keep_alive = true;
// 		if (this->_header_kv["Connection"] == "close")
// 			this->_should_keep_alive = false;
// 	}
// 	else if (this->_header_kv["Version"] == "HTTP/1.0") 	// HTTP/1.0 requires explicit keep-alive
// 	{
// 		this->_should_keep_alive = false;
// 		if (this->_header_kv["Connection"] == "keep-alive")
// 			this->_should_keep_alive = true;
// 	}

// 	std::map<std::string, std::string>::iterator it;
// 	for (it = this->_header_kv.begin(); it != this->_header_kv.end(); ++it) {
// 			std::cout << "'" << it->first << "' : '" << it->second << "'" << std::endl;
// 	}
// }

// void Request::parse_body()
// {
// 	/* WIP */
// 	this->_work_request = true;
// }

// void process_request(Request &req)
// {
// 	std::cout << "\033[34mProcessing request of type: "<< req._header_kv["Method"] <<"\n\033[0m" << std::endl;
// 	req._status_code = 200;

// 	// Assumes the request was fully parsed
// 	// We then check what the request wants (POST, GET, DELETE etc) and process accordingly
// 	// After we have processed the request, we set the status code
// 	// we return to the poll loop where we will send the response based on the status code
// }


