#ifndef RESPONSE_H
# define RESPONSE_H

class Client;

# include "../Functions/requestUtils.hpp"
# include <string>
# include <vector>
# include <map>
# include <iostream>
#	include <sstream>
# include <ctime>

class Response
{
	private:
		std::string _header;
		size_t _content_length;
		size_t _bytes_sent;
		std::string _response_data;

	public:
		std::string _body; // TODO: move to Private
		std::vector <std::string>	_allowedMethods; // TODO: move to Private

		Response();
		Response(const Response& other);
		Response& operator=(const Response& other);
		~Response();

		std::string format_response(int status_code, bool should_keep_alive, std::string version, std::string body);
		std::string get_response_data() const { return _response_data; };
		void set_response_data(std::string str) { _response_data = str; };
		std::string get_body() const { return _body; };
		void flush_response_data();
};


#endif
