#ifndef RESPONSE_H
#define RESPONSE_H

class Client;

#include "../Functions/requestUtils.hpp"
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <ctime>

class Response
{
private:
	std::string _header;
	size_t _content_length;
	size_t _bytes_sent;
	std::string _response_data;
	std::string _body;
	std::vector<std::string> _allowedMethods;

public:
	Response();
	Response(const Response &other);
	Response &operator=(const Response &other);
	~Response();

	std::string format_response(int status_code, bool should_keep_alive, std::string version, std::string body);

	std::string get_header() const { return _header; };
	size_t get_content_length() const { return _content_length; };
	size_t get_bytes_sent() const { return _bytes_sent; };
	std::string get_response_data() const { return _response_data; };
	std::string get_body() const { return _body; };
	std::vector<std::string> get_allowed_methods() const { return _allowedMethods; };

	void set_header(std::string header) { _header = header; };
	void set_content_length(size_t content_length) { _content_length = content_length; };
	void set_bytes_sent(size_t bytes_sent) { _bytes_sent = bytes_sent; };
	void set_response_data(std::string response_data) { _response_data = response_data; };
	void set_body(std::string body) { _body = body; };
	void set_allowed_methods(std::vector<std::string> allowedMethods) { _allowedMethods = allowedMethods; };

	void flush_response_data();
};

#endif
