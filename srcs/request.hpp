#ifndef REQUEST_H
# define REQUEST_H

# include <string>
# include <vector>

class Request
{
	private:

	public:
		std::string _request_data;
		std::string _header;
		std::string _body;
		size_t _content_length;
		bool _header_parsed;
		bool _work_request;
		int _status_code;
		int _inORout;

		Request();
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

};

#endif
