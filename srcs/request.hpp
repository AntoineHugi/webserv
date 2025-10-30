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
		size_t _content_length = 0;
		bool _header_parsed = false;
		bool _work_request = false;

		Request();
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

};

#endif
