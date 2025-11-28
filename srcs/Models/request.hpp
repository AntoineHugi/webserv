#ifndef REQUEST_H
#define REQUEST_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cctype>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <algorithm>

#include "multipart.hpp"

class Request
{
	private:
		// size_t _reading_lenght;
	public:
		/* header attributes */
		std::string _request_data;
		std::string _header;
		std::string _method;
		std::string _uri;
		std::string _version;
		std::string _host;
		std::map<std::string, std::string> _header_kv;
		size_t _content_length;
		std::string _fullPathURI;
		std::string _root;
		bool _isDirectory;
		struct stat _stat;
		bool _isCGI;
		unsigned long	_client_max_body_size;
		std::vector<std::string> _index;
		std::string _cgi_path;

		/* body attributes */
		std::string _body;
		std::map<std::string, std::string> _body_kv;
		std::vector<MultiPart> _multiparts;
		std::vector<char> _body_data;

		Request();
		Request(const Request &other);
		Request &operator=(const Request &other);
		~Request();

		void	flush_request_data();

		/* Header parsing */
		int		parse_header();
		int		http_requirements_met();
		bool	http_can_have_body();

		/* Body parsing */
		int		parse_body();
		int		parse_url_encoded();
		std::vector<std::string>	tokenise_url_encoded(std::string &str);
		int		parse_multipart(std::string content_type);
		std::string	find_boundary(std::string content_type);
		std::vector<MultiPart>	generate_multipart(const std::string &boundary);
		std::string	trimCRLF(const std::string &s);
		int		parse_json();
		std::vector<std::string>	tokenise_json(std::string &str);
		int		treat_as_raw_body();

		// size_t get_reading_length() const { return _reading_lenght; };
		// void set_reading_length(size_t length) { _reading_lenght = length; };
};

#endif
