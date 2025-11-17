#ifndef REQUEST_H
# define REQUEST_H

# include <string>
# include <vector>
# include <map>
# include <iostream>
# include <sys/stat.h>

class Request
{
	private:
		// size_t _reading_lenght;
	public:
		std::string	_request_data;
		std::string	_header;
		std::string	_body;
		std::map<std::string, std::string> _header_kv;
		size_t	_content_length;
		std::string	_fullPathURI;
		std::string	_root;
		bool	_isDirectory;
		struct stat	_stat;
		Request();
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

		void flush_request_data();
		int parse_header();
		void parse_body();
		int http_requirements_met();
		bool http_can_have_body();
		// size_t get_reading_length() const { return _reading_lenght; };
		// void set_reading_length(size_t length) { _reading_lenght = length; };
};

#endif
