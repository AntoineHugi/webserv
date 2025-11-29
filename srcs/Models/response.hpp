#ifndef RESPONSE_H
#define RESPONSE_H

class Client;

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <ctime>

#include "request.hpp"

class Response
{
private:
	std::string _header;
	size_t _content_length;
	size_t _bytes_sent;
	std::string _response_data;
	std::string _body;
	std::vector<std::string> _allowedMethods;
	std::string _content_type;
	std::string _location;

public:
	Response();
	Response(const Response &other);
	Response &operator=(const Response &other);
	~Response();

	std::string& get_header() { return _header; };
	size_t get_content_length() const { return _content_length; };
	size_t get_bytes_sent() const { return _bytes_sent; };
	std::string get_response_data(int start) const { return _response_data.substr(start); };
	std::string& get_response_data_full() { return _response_data; };
	std::string& get_body() { return _body; };
	std::vector<std::string> get_allowed_methods() const { return _allowedMethods; };
	std::string& get_content_type() { return _content_type; };
	std::string& get_location() { return _location; };

	void update_bytes_sent(int res) { _bytes_sent += res; };

	void set_header(std::string header) { _header = header; };
	void set_content_length(size_t content_length) { _content_length = content_length; };
	void set_bytes_sent(size_t bytes_sent) { _bytes_sent = bytes_sent; };
	void set_response_data(std::string& response_data) { _response_data = response_data; };
	void set_body(std::string& body) { _body = body; };
	void set_allowed_methods(std::vector<std::string> allowedMethods) { _allowedMethods = allowedMethods; };
	void set_content_type(std::string content_type) { _content_type = content_type; };
	void set_location(std::string& location) { _location = location; };

	void flush_response_data();
	void format_response(int status_code, bool should_keep_alive, std::string version);
	std::string get_reason_phrase(int status_code);
	std::string capitalize_header(const std::string &name);
	std::map<std::string, std::string> parse_cgi_headers(const std::string &header_text);
};

#endif
