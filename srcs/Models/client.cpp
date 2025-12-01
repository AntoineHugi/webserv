#include "client.hpp"
#include "clientUtils.cpp"
#include "clientParsing.cpp"

Client::Client() :
	_state(READING_HEADERS),
	_flags(),
	_fd(-1),
	_status_code(200),
	_last_interaction(std::time(0)),
	_server(),
	_request(),
	_response()
{}

Client::Client(const Client &other) :
	_state(other._state),
	_flags(other._flags),
	_fd(other._fd),
	_status_code(other._status_code),
	_last_interaction(other._last_interaction),
	_server(other._server),
	_client_ip(other._client_ip),
	_request(other._request),
	_response(other._response)
{}

Client &Client::operator=(const Client &other)
{
	if (this != &other)
	{
		_state = other._state;
		_flags = other._flags;
		_fd = other._fd;
		_status_code = other._status_code;
		_last_interaction = other._last_interaction;
		_server = other._server;
		_client_ip = other._client_ip;
		_request = other._request;
		_response = other._response;
	}
	return *this;
}

Client::Client(int fd, Server &server, std::string client_ip) :
	_state(READING_HEADERS),
	_flags(),
	_fd(fd),
	_status_code(200),
	_last_interaction(std::time(0)),
	_server(&server),
	_client_ip(client_ip),
	_request(),
	_response()
{}

Client::~Client() {}

/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/

int Client::handle_read()
{
	if (!can_i_read_header() && !can_i_read_body())
	{
		std::cout << "@@@@@@  IMPOSSIBLE >>>>>>>>>> Client not in a reading state!" << std::endl;
		_status_code = 500;
		set_process_request();
		return 1;
	}

	if (!leftover_chunk())
	{
		int read_result = read_to_buffer();
		if (read_result == -1)
		{
			_status_code = 500;
			set_process_request();
			return 1;
		}

		/* proceed to next step if we received data */
		if (read_result == 0)
			return 1;
	}
	else
		_flags._leftover_chunk = false;

	/* attempt finding and parsing header on what we received */
	if (can_i_read_header())
	{
		if (try_parse_header() == 1)
			return (1);
		else
		{
			if (can_i_process_request())
				return (2);
		}
	}

	if (can_i_read_body())
	{
		if (try_parse_body() == 1)
			return 1;
		else
		{
			if (can_i_process_request())
				return (2);
		}
	}
	return 0;
}

int Client::process_request()
{
	std::string methods[3] = {"GET", "POST", "DELETE"};
	int field = -1;
	for (int i = 0; i < 3; i++)
	{
		if (_request._method == methods[i])
			field = i;
	}
	switch (field)
	{
	case 0:
		return (Method::handle_get(*this));
	case 1:
		Method::handle_post(*this);
		break;
	case 2:
		Method::handle_delete(*this);
		break;
	default:
		if (DEBUG)
			std::cout << "Error processing request, method is = " << _request._method << std::endl;
		set_status_code(500);
		return (-1);
	}
	return (0);
}

int Client::handle_write()
{
	if (!can_i_create_response() && !can_i_send_response())
	{
		std::cout << "@@@@@@  IMPOSSIBLE >>>>>>>>>> Client not geting a reponse!" << std::endl;
		_status_code = 500;
		create_default_error();
		set_create_response();
		return 1;
	}
	if (_status_code >= 400)
		set_flags_error();
	if (can_i_create_response())
	{
		_response.format_response(get_status_code(), should_keep_alive(), _request.get_version());
		set_send_response();
	}
	if (DEBUG)
		std::cout << "==>>  Client will receive answer" << std::endl;
	std::string res = _response.get_response_data(_response.get_bytes_sent()).substr(0, BUFFER_SIZE);
	if (_response.get_bytes_sent() == 0)
	{
		if (DEBUG)
		{
			std::cout << "Response to be sent:\n-----\n"
				  << res.substr(0, 2048) << "-----\n\n"
				  << std::endl;
		}
	}

	ssize_t bytes_sent = send(_fd, res.c_str(), res.size(), 0);
	if (bytes_sent == -1)
	{
		_status_code = 500;
		set_flags_error();
		if (DEBUG)
			std::cout << "Error sending response" << std::endl;
		return (1);
	}
	_response.update_bytes_sent(bytes_sent);
	if (DEBUG)
		std::cout << "bytes_sent(): " << _response.get_bytes_sent() << " out of " << _response.get_response_data_full().size() << std::endl;
	if (_response.get_bytes_sent() == (size_t)_response.get_response_data_full().size())
	{
		if (DEBUG)
			std::cout << "I'm changing status" << std::endl;
		if (_flags._should_keep_alive)
		{
			if (DEBUG)
				std::cout << "I'm staying alive" << std::endl;
			set_finish_request_alive();
		}
		else
		{
			if (DEBUG)
				std::cout << "I'm closing" << std::endl;
			set_finish_request_close();
		}
	}
	return (0);
}
