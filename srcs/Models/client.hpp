#ifndef CLIENT_H
# define CLIENT_H

# include "../Models/request.hpp"
# include "../Models/response.hpp"
# include "../Models/server.hpp"
# include "../Functions/requestUtils.hpp"
#	include "../Core/service.hpp"

# include <string>
# include <vector>
# include <map>
#	include <iostream>
#	include <sys/socket.h>
#	include <poll.h>
#	include <cstring>
#	include <cerrno>
#	include <stdio.h>
#	include <errno.h>

class Client
{
	private:
		enum State {
			READING_HEADERS,
			READING_BODY,
			PROCESSING_REQUEST,
			SENDING_RESPONSE,
			KEEPALIVE_WAIT,
			CLOSING
		};
		State _state;
		int _fd;
		Server* _server;
		int _status_code;

	public:
		// bool _header_parsed;
		// bool _work_request;
		// bool _inORout;
		bool _should_keep_alive;
		Request _request;
		Response _response;

		Client();
		Client(int fd, Server& server);
		Client(const Client& other);
		Client& operator=(const Client& other);
		~Client();

		int return_set_status_code(int code);

		int	handle_read();
		void handle_write();
		int fill_request_data();

		int get_fd() const { return _fd; };
		int get_status_code() const { return _status_code; };
		void set_status_code(int code) { _status_code = code; };

		enum State get_state() const { return _state; };
		bool can_i_read_header() const { return _state == READING_HEADERS; };
		bool can_i_read_body() const { return _state == READING_BODY; };
		bool can_i_process_request() const { return _state == PROCESSING_REQUEST; };
		bool can_i_send_response() const { return _state == SENDING_RESPONSE; };
		void set_read_header() { _state = READING_HEADERS; };
		void set_read_body() { _state = READING_BODY; };
		void set_process_request() { _state = PROCESSING_REQUEST; };
		void set_send_response() { _state = SENDING_RESPONSE; };

		void set_flags();

		// void process_request(Request &req);
		// std::string format_response(Request &req);
};

// int fill_request_data(int fd, Client& client);
// std::string format_response(Client &client);

#endif
