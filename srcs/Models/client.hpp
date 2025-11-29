#ifndef CLIENT_H
# define CLIENT_H

# include "../Models/request.hpp"
# include "../Models/response.hpp"
# include "../Models/server.hpp"
# include "../Functions/method.hpp"

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

class Client
{
	private:
		enum State {
			READING_HEADERS,
			READING_BODY,
			PROCESSING_REQUEST,
			CREATING_RESPONSE,
			WAITING_CGI,
			SENDING_RESPONSE,
			KEEPALIVE_WAIT,
			CLOSING,
			HANDLE_ERROR
		};
		State _state;

		struct flags {
			bool _should_keep_alive;
			bool _body_chunked;
			bool _leftover_chunk;

			flags():
				_should_keep_alive(false),
				_body_chunked(false),
				_leftover_chunk(false)
				{};

			flags(const Client::flags& other_flags):
				_should_keep_alive(other_flags._should_keep_alive),
				_body_chunked(other_flags._body_chunked),
				_leftover_chunk(other_flags._leftover_chunk)
				{};

		} _flags;

		int _fd;
		int _status_code;
		std::time_t _last_interaction;
		Server* _server;
		std::string _client_ip;

	public:
		Request _request;
		Response _response;

		Client();
		Client(int fd, Server& server, std::string client_ip);
		Client(const Client& other);
		Client& operator=(const Client& other);
		~Client();

		int get_fd() const { return _fd; };
		int get_status_code() const { return _status_code; };
		Server* get_server() const { return _server; };
		enum State get_state() const { return _state; };
		bool can_i_read_header() const { return _state == READING_HEADERS; };
		bool can_i_read_body() const { return _state == READING_BODY; };
		bool can_i_process_request() const { return _state == PROCESSING_REQUEST; };
		bool can_i_create_response() const { return _state == CREATING_RESPONSE; };
		bool am_i_waiting_cgi() const { return _state == WAITING_CGI; };
		bool can_i_send_response() const { return _state == SENDING_RESPONSE; };
		bool can_i_close_connection() const { return _state == CLOSING; };
		bool is_error() const { return _state == HANDLE_ERROR; };
		bool is_inactive() const { return std::time(0) - _last_interaction >= CLIENT_TIMEOUT_MS/1000; };

		void set_status_code(int code) { _status_code = code; };
		void set_read_header() { _state = READING_HEADERS; };
		void set_read_body() { _state = READING_BODY; };
		void set_process_request() { _state = PROCESSING_REQUEST; };
		void set_create_response() { _state = CREATING_RESPONSE; };
		void set_wait_cgi() { _state = WAITING_CGI; };
		void set_send_response() { _state = SENDING_RESPONSE; };
		void set_finish_request_alive() {_state = KEEPALIVE_WAIT; };
		void set_finish_request_close() {_state = CLOSING; };
		void set_handle_error() {_state = HANDLE_ERROR; };
		void update_last_interaction() { _last_interaction = std::time(0);}

		/* Read section */
		int		handle_read();
		int		read_to_buffer();
		bool	try_parse_header();
		bool	try_parse_body();
		bool	chunked_body_finished() const;
		bool	decode_chunked_body();
		bool	validate_permissions();
		int		find_best_route_index(std::vector<Route>& routes);
		bool	cgi_matches(const std::string &uri, const std::string &route);
		std::vector<std::string> fetch_extensions(const std::string &cgi_path);
		bool	route_matches(const std::string &uri, const std::string &route);
		void	overwrite_with_route(const Route& route);
		bool	check_uri_exists();
		bool	bouncer_approval(const Route &route);
		bool	check_subnet(const std::string& rule_target, const std::string& _client_ip);
		bool	check_directory_rules(const Route &route);
		bool	is_method_allowed(const Route &route);
		bool	validate_methods();
		bool	transversal_protection();

		/* Processing section */
		void	process_request();

		/* Writing section */
		int		handle_write();

		/* Utils section */
		void	set_flags();
		void	set_flags_error();
		void	refresh_client();
		bool	is_body_chunked() { return _flags._body_chunked; };
		bool	should_keep_alive() const { return _flags._should_keep_alive; };
		bool	leftover_chunk() const { return _flags._leftover_chunk; };
		bool	request_complete() const { return _state == PROCESSING_REQUEST; };
};


#endif
