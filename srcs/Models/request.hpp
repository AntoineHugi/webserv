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

extern bool DEBUG;
extern const size_t BUFFER_SIZE;
extern int CLIENT_TIMEOUT_MS;

class Request
{
	private:
		std::string _version;

	public:
		/* header attributes */
		std::string _request_data;
		std::string _header;
		std::string _method;
		std::string _uri;
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
		bool	_autoindex;

		/* body attributes */
		std::string _body;
		std::map<std::string, std::string> _body_kv;
		std::vector<MultiPart> _multiparts;
		std::vector<char> _body_data;

		/* for GET */
		std::string	_filepath;

		Request();
		Request(const Request &other);
		Request &operator=(const Request &other);
		~Request();

		// Getters
		std::string	get_version() const { return _version; };
		size_t		get_content_length() const { return _content_length; };

		// Setters
		void	set_version(const std::string &version) { _version = version; };
		void	set_content_length(size_t length) { _content_length = length; };


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
		int		treat_as_raw_body();
};

#endif
