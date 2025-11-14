#ifndef CLIENT_H
# define CLIENT_H

# include <string>
# include <vector>
# include <map>
# include <iostream>
# include <sys/socket.h>
# include <poll.h>
# include <cstring>
# include <cerrno>
# include <stdio.h>
# include <errno.h>
# include <sstream>

# include "request.hpp"
# include "response.hpp"
# include "server.hpp"

class Client
{
	private:

	public:
		Request _request;
		Response _response;
		bool _header_parsed;
		bool _work_request;
		int _status_code;
		int _fd;
		bool _inORout;
		bool _should_keep_alive;
		Server _server;

		Client();
		Client(const Client& other);
		Client& operator=(const Client& other);
		~Client();

		int	handle_read();
		void handle_write();

		void parse_header();
		void parse_body();

		// void process_request(Request &req);
		// std::string format_response(Request &req);
};

int fill_request_data(int fd, Client& client);
std::string format_response(Client &client);

#endif
