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
		std::string _uri;
		std::string _method;
		std::string _host;
		std::string _header;
		size_t _content_length;
		std::string _root;
		std::string _fullPathURI;
		std::string _request_data;
		bool _isDirectory;
		bool _isCGI;
		bool	_autoindex;
		std::string _cgi_path;
		unsigned long	_client_max_body_size;
		std::string _body;

	public:
		std::map<std::string, std::string> _header_kv;
		struct stat _stat;
		std::vector<std::string> _index;

		std::map<std::string, std::string> _body_kv;
		std::vector<MultiPart> _multiparts;
		std::vector<char> _body_data;

		Request();
		Request(const Request &other);
		Request &operator=(const Request &other);
		~Request();

		// Getters
		std::string	get_version() const { return _version; };
		std::string	get_uri() const { return _uri; };
		std::string	get_method() const { return _method; };
		// std::string	get_host() const { return _host; };
		std::string	get_header() const { return _header; };
		size_t		get_content_length() const { return _content_length; };
		std::string	get_root() const { return _root; };
		std::string	get_fullPathURI() const { return _fullPathURI; };
		std::string	get_request_data() const { return _request_data; };
		std::string	get_cgi_path() const { return _cgi_path; };
		unsigned long	get_client_max_body_size() const { return _client_max_body_size; };
		std::string	get_body() const { return _body; };

		bool _is_directory() const { return _isDirectory; };
		bool _is_cgi() const { return _isCGI; };
		bool _is_autoindex() const { return _autoindex; };
		// std::map<std::string, std::string>	get_header_kv() const { return _header_kv; };
		// std::string get_header_kv_value(const std::string &key) const { return _header_kv.count(key) ? _header_kv.at(key) : ""; };

		// Setters
		void set_header(const std::string &header) { _header = header; };
		// void	set_version(const std::string &version) { _version = version; };
		// void	set_uri(const std::string &uri) { _uri = uri; };
		void	set_content_length(size_t length) { _content_length = length; };
		void set_root(const std::string &root) { _root = root; };
		void set_fullPathURI(const std::string &fullPathURI) { _fullPathURI = fullPathURI; };
		void set_request_data(const std::string &data) { _request_data = data; };
		void set_is_directory(bool isDir) { _isDirectory = isDir; };
		// void set_header_kv_value(const std::string &key, const std::string &value) { _header_kv[key] = value; };
		void set_is_cgi(bool isCGI) { _isCGI = isCGI; };
		void set_is_autoindex(bool autoindex) { _autoindex = autoindex; };
		void set_cgi_path(const std::string &cgi_path) { _cgi_path = cgi_path; };
		void set_client_max_body_size(unsigned long size) { _client_max_body_size = size; };
		void set_body(const std::string &body) { _body = body; };


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

};

#endif
