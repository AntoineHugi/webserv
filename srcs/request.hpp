#ifndef REQUEST_H
# define REQUEST_H

# include <string>
# include <vector>
# include <map>
# include <iostream>

class Request
{
	private:

	public:
		std::string	_request_data;
		std::string	_header;
		std::string	_body;
		std::map<std::string, std::string> _header_kv;
		size_t	_content_length;
		std::string	_fullPathURI;
		std::string	_root;
		bool	_isDirectory;
		Request();
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

		void flush_request_data();
};	

#endif
