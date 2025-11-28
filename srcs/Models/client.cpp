#include "client.hpp"
#include "clientUtils.cpp"
#include "clientParsing.cpp"

Client::Client() : _state(READING_HEADERS),
		   _flags(),
		   _fd(-1),
		   _status_code(200),
		   _last_interaction(std::time(0)),
		   _server(),
		   _request(),
		   _response() {}

Client::Client(int fd, Server &server, std::string client_ip) : _state(READING_HEADERS),
							_flags(),
							_fd(fd),
							_status_code(200),
							_last_interaction(std::time(0)),
							_server(&server),
							_client_ip(client_ip),
							_request(),
							_response() {}

Client::Client(const Client &other) : _state(other._state),
				      _flags(other._flags),
				      _fd(other._fd),
				      _status_code(other._status_code),
				      _last_interaction(other._last_interaction),
				      _server(other._server),
				      _client_ip(other._client_ip),
				      _request(other._request),
				      _response(other._response) {}

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
		set_create_response();
		return 1;
	}

	if (!leftover_chunk())
	{
		// std::cout << "request data was empty" << std::endl;
		int read_result = read_to_buffer();
		if (read_result == -1)
		{
			_status_code = 500;
			set_create_response();
			return 1;
		}

		/* proceed to next step if we received data */
		if (read_result == 0)
		{
			return 1;
		}
	}
	else
		_flags._leftover_chunk = false;

	/* attempt finding and parsing header on what we received */
	if (can_i_read_header())
	{
		if (try_parse_header() == 1)
		{
			if (get_status_code() > 400)
			{
				int status_code = get_status_code();
				if (!get_server()->get_error_page().empty() && !get_server()->get_error_page()[status_code].empty())
				{
					Method::get_file(*this, get_server()->get_error_page()[status_code]);
					set_status_code(status_code);
				}
			}
			return (1);
		}

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

void Client::process_request()
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
		Method::handle_get(*this);
		break;
	case 1:
		Method::handle_post(*this);
		break;
	case 2:
		Method::handle_delete(*this);
		break;
	default:
		std::cout << "Error processing request, method is = " << _request._method << std::endl;
		set_status_code(500);
		return;
	}
}

int Client::handle_write()
{
	if (!can_i_create_response() && !can_i_send_response())
	{
		std::cout << "@@@@@@  IMPOSSIBLE >>>>>>>>>> Client not geting a reponse!" << std::endl;
		_status_code = 500;
		set_create_response();
		return 1;
	}
	if (_status_code >= 400)
		set_flags_error();
	_response.set_request(&_request);
	std::string response = _response.format_response(get_status_code(), should_keep_alive(), _request._version);
	_response.set_response_data(response);
	set_send_response();
	std::cout << "==>>  Client will receive answer" << std::endl;
	std::string res = _response.get_response_data(_response.get_bytes_sent()).substr(0, 1048576);
	if (_response.get_bytes_sent() == 0)
	{
		std::cout << "Response to be sent:\n-----\n"
				<< res.substr(0,2048) << "-----\n\n"
				<< std::endl;
	}


	ssize_t bytes_sent = send(_fd, res.c_str(), res.size(), 0);
	if (bytes_sent == -1)
	{
		_status_code = 500;
		set_flags_error();
		std::cout << "Error sending response" << std::endl;
		return (1);
	}
	_response.update_bytes_sent(bytes_sent);
	// std::cout << "bytes sent: " << bytes_sent << std::endl;
	std::cout << "bytes_sent(): " << _response.get_bytes_sent() << " out of " << _response.get_response_data_full().size() << std::endl;
	if (_response.get_bytes_sent() == (size_t)_response.get_response_data_full().size())
	{
		std::cout << "I'm changing status" << std::endl;
		if (_flags._should_keep_alive)
		{
			std::cout << "I'm staying alive" << std::endl;
			set_finish_request_alive();
		}
		else
		{
			std::cout << "I'm closing" << std::endl;
			set_finish_request_close();
		}
	}
	return (0);
}

/*

	## HTTP Response Structure

	### Format
	```
	<VERSION> <STATUS_CODE> <REASON_PHRASE>\r\n
	<Header-Name>: <Header-Value>\r\n
	<Header-Name>: <Header-Value>\r\n
	...
	\r\n
	<BODY>


	## Response Status Line (Mandatory)
	```
	VERSION STATUS_CODE REASON_PHRASE\r\n
	```

	**VERSION:** Same as request (`HTTP/1.1`)

	**STATUS_CODE:** 3-digit number indicating result

	**REASON_PHRASE:** Human-readable description (optional but conventional)

	---

	## Response Headers (What You Should Include)

	### Mandatory/Highly Recommended Headers

	**Content-Length** (If you know body size)
	```
	Content-Length: 1234
	```
	Tells client exactly how many bytes to expect.

	**Content-Type** (Describes body format)
	```
	Content-Type: text/html; charset=utf-8
	Content-Type: application/json
	Content-Type: image/png
	```

	**Connection** (Control keep-alive)
	```
	Connection: close        # Close after sending response
	Connection: keep-alive   # Keep connection open
	```

	**Date** (Current server time - recommended)
	```
	Date: Wed, 21 Oct 2024 07:28:00 GMT
	```

	**Server** (Your server identification - optional)
	```
	Server: webserv/1.0
	```

	### Optional But Useful Headers

	**Location** (For redirects)
	```
	HTTP/1.1 301 Moved Permanently
	Location: https://newsite.com/page
	```

	**Set-Cookie** (Send cookie to client)
	```
	Set-Cookie: session_id=abc123; Path=/; HttpOnly
	```

	**Cache-Control** (Control caching)
	```
	Cache-Control: no-cache
	Cache-Control: max-age=3600
	```

	**Content-Encoding** (If you compress body)
	```
	Content-Encoding: gzip

	Response Body
	Body Depends on Status Code and Content-Type
	200 OK with HTML:
	httpHTTP/1.1 200 OK
	Content-Type: text/html
	Content-Length: 50

	<html><body><h1>Hello</h1></body></html>
	200 OK with JSON:
	httpHTTP/1.1 200 OK
	Content-Type: application/json
	Content-Length: 25

	{"status":"success"}
	404 Not Found:
	httpHTTP/1.1 404 Not Found
	Content-Type: text/html
	Content-Length: 45

	<html><body><h1>404 Not Found</h1></body></html>
	204 No Content (No body!):
	httpHTTP/1.1 204 No Content
	Content-Length: 0

*/
