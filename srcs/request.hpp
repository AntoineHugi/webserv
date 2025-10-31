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
		std::string _request_data;
		std::string _header;
		std::string _body;
		size_t _content_length;
		bool _header_parsed;
		bool _work_request;
		int _status_code;
		bool _inORout;
		bool _should_keep_alive;
		std::map<std::string, std::string> _header_kv;

		Request();
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();


};

	void parse_header(Request &req);
	void parse_body(Request &req);
	void process_request(Request &req);
	void flush_request_data(Request &req);
	std::string format_response(Request &req);

#endif
