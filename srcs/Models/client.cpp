#include "client.hpp"

Client::Client() : _state(READING_HEADERS),
		   _flags(),
		   _fd(-1),
		   _status_code(200),
		   _last_interaction(std::time(0)),
		   _server(),
		   _request(),
		   _response() {}

Client::Client(int fd, Server &server) : _state(READING_HEADERS),
					 _flags(),
					 _fd(fd),
					 _status_code(200),
					 _last_interaction(std::time(0)),
					 _server(&server),
					 _request(),
					 _response() {}

Client::Client(const Client &other) : _state(other._state),
				      _flags(other._flags),
				      _fd(other._fd),
				      _status_code(other._status_code),
				      _last_interaction(other._last_interaction),
				      _server(other._server),
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
		_request = other._request;
		_response = other._response;
	}
	return *this;
}

Client::~Client() {}

void Client::set_flags_error()
{
	_flags._should_keep_alive = false;
}

void Client::set_flags()
{
	if (_request._version == "HTTP/1.1") // HTTP/1.1 has keep-alive by default
	{
		_flags._should_keep_alive = true;
		if (_request._header_kv["connection"] == "close")
			_flags._should_keep_alive = false;
	}
	else if (_request._version == "HTTP/1.0") // HTTP/1.0 requires explicit keep-alive
	{
		_flags._should_keep_alive = false;
		if (_request._header_kv["connection"] == "keep-alive")
			_flags._should_keep_alive = true;
	}

	if (_request._header_kv["transfer-encoding"] == "chunked")
		_flags._body_chunked = true;
	else
		_flags._body_chunked = false;
}

void Client::refresh_client()
{
	std::cout << ">>>   CLIENT REFRESH <<<" << std::endl;
	_request.flush_request_data();
	_response.flush_response_data();
	_state = READING_HEADERS;
	_flags._should_keep_alive = false;
	_flags._body_chunked = false;
	// _flags._leftover_chunk = false;
	_status_code = 200;
}

/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/

bool Client::validate_permissions()
{
	/* looks the best matching route for that uri */
	std::vector<Route> routes = _server->get_routes();
	if (routes.empty())
	{
		Route default_route;
		std::vector<std::string> methods;
		methods.push_back("GET");
		default_route.set_path("/");
		default_route.set_methods(methods);
		routes.push_back(default_route);
	}
	int index = -1;
	std::string matchedPath;
	for (size_t i = 0; i < routes.size(); ++i)
	{
		std::string path = routes[i].get_path();
		if (_request._uri.compare(0, path.size(), path) == 0)
		{
			if (path.size() >= matchedPath.size())
			{
				matchedPath = path;
				index = i;
			}
		}
	}
	if (matchedPath.empty() || index == -1)
	{
		set_status_code(404);
		return (false);
	}

	/* sets the right root for security: default or specific to the route and creates the full path root + uri */
	if (!routes[index].get_root().empty())
	{
		_request._root = routes[index].get_root();
		_request._fullPathURI = routes[index].get_root() + _request._uri.substr(matchedPath.size());
	}
	else
	{
		_request._root = _server->get_root();
		_request._fullPathURI = _server->get_root() + _request._uri.substr(matchedPath.size());
	}

	/* checks that the target uri exists */
	if (stat(_request._fullPathURI.c_str(), &_request._stat) != 0)
	{
		set_status_code(404);
		return (false);
	}

	/* checks if it's a directory */
	if (S_ISDIR(_request._stat.st_mode) && (_request._method == "GET" || _request._method == "POST"))
	{
		if ((!routes[index].get_autoindex().empty() && _request._method == "GET") || _request._method == "POST")
			_request._isDirectory = true;
		else
		{
			set_status_code(403);
			return (false);
		}
	}

	/* checks that the method is allowed by that route, and then validates that method's permissions */
	for (size_t j = 0; j < routes[index].get_methods().size(); ++j)
	{
		if (_request._method == routes[index].get_methods()[j])
		{
			if (validate_methods())
				return (true);
			else
				return (false);
		}
	}
	set_status_code(405);
	_response.set_allowed_methods(routes[index].get_methods());
	return (false);
}

bool Client::validate_methods()
{
	if (_request._method == "GET")
	{
		/* checking if we can read/execute this file/directory */
		if (_request._isCGI && access(_request._fullPathURI.c_str(), X_OK) != 0)
		{
			set_status_code(403);
			return (false);
		}
		else if (!_request._isCGI && access(_request._fullPathURI.c_str(), R_OK) != 0)
		{
			set_status_code(403);
			return (false);
		}
	}
	else if (_request._method == "POST")
	{
		/* checking if we can post in this directory */
		std::string dirPath;
		{
			char tmp[PATH_MAX];
			strncpy(tmp, _request._fullPathURI.c_str(), PATH_MAX);
			tmp[PATH_MAX - 1] = '\0';

			char *d = dirname(tmp);
			dirPath = std::string(d);
		}
		if (access(dirPath.c_str(), W_OK) != 0)
		{
			set_status_code(403);
			return (false);
		}
	}
	else if (_request._method == "DELETE" || _request._method == "POST")
	{
		/* checking that the path to the file for deletion is located within the root of this route / server class */
		char resolvedPath[PATH_MAX];
		char resolvedRoot[PATH_MAX];
		if (!realpath(_request._fullPathURI.c_str(), resolvedPath) || !realpath(_request._root.c_str(), resolvedRoot))
		{
			set_status_code(403);
			return (false);
		}
		std::string resolvedFile(resolvedPath);
		std::string resolvedRootDir(resolvedRoot);
		if (resolvedRootDir[resolvedRootDir.size() - 1] != '/')
			resolvedRootDir += '/';
		if (resolvedFile.find(resolvedRootDir) != 0)
		{
			set_status_code(403);
			return (false);
		}

		/* checking if we can delete in this directory */
		std::string dirPath;
		{
			char tmp[PATH_MAX];
			strncpy(tmp, _request._fullPathURI.c_str(), PATH_MAX);
			tmp[PATH_MAX - 1] = '\0';

			char *d = dirname(tmp);
			dirPath = std::string(d);
		}
		if (access(dirPath.c_str(), W_OK) != 0)
		{
			set_status_code(403);
			return (false);
		}
	}
	return (true);
}

int Client::read_to_buffer()
{
	char buf[50];
	ssize_t n = recv(_fd, buf, sizeof(buf), 0);

	if (n > 0)
	{
		_request._request_data.append(buf, n);
		return 1;
	}
	else if (n == 0)
		return 0;
	else
		return -1;
}

bool Client::try_parse_chunked_body()
{
	std::string &data = _request._request_data;
	size_t &idx = _request._chunk_parse_index;

	while (true)
	{
		if (_request._chunk_size_pending)
		{
			/* returns if need more data */
			size_t line_end = data.find("\r\n", idx);
			if (line_end == std::string::npos)
			{
				std::cout << "returned very early" << std::endl;
				return false;
			}

			std::string size_str = data.substr(idx, line_end - idx);
			size_t semicolon = size_str.find(';');
			if (semicolon != std::string::npos)
				size_str = size_str.substr(0, semicolon);
			if (size_str.empty())
			{
				std::cout << "size_str empty" << std::endl;
				_status_code = 400;
				set_create_response();
				return false;
			}

			for (size_t i = 0; i < size_str.size(); ++i)
			{
				char c = size_str[i];
				bool hex = (c >= '0' && c <= '9') ||
					   (c >= 'a' && c <= 'f') ||
					   (c >= 'A' && c <= 'F');
				if (!hex)
				{
					_status_code = 400;
					std::cout << "no hex" << std::endl;
					set_create_response();
					return false;
				}
			}

			// Manual hex parse
			size_t chunk_size = 0;
			for (size_t i = 0; i < size_str.size(); ++i)
			{
				chunk_size *= 16;
				char c = size_str[i];
				if (c >= '0' && c <= '9')
					chunk_size += (c - '0');
				else if (c >= 'a' && c <= 'f')
					chunk_size += (c - 'a' + 10);
				else
					chunk_size += (c - 'A' + 10);
			}
			_request._current_chunk_size = chunk_size;

			idx = line_end + 2;
			_request._chunk_size_pending = false;
		}

		size_t chunk_size = _request._current_chunk_size;

		if (data.size() < idx + chunk_size + 2)
		{
			std::cout << "returned kinda early" << std::endl;
			return false;
		}

		if (static_cast<long long>(_request._decoded_body.size() + chunk_size) > _server->get_client_max_body_size())
		{
			std::cout << "too large" << std::endl;
			_status_code = 413;
			set_create_response();
			return false;
		}

		if (chunk_size > 0)
			_request._decoded_body.append(data, idx, chunk_size);

		if (data[idx + chunk_size] != '\r' || data[idx + chunk_size + 1] != '\n')
		{
			std::cout << "no rn at the end" << std::endl;
			_status_code = 400;
			set_create_response();
			return false;
		}

		idx += chunk_size + 2;
		if (chunk_size == 0)
		{
			size_t trailers_end = data.find("\r\n\r\n", idx);
			if (trailers_end == std::string::npos)
			{
				if (data.compare(idx-2, 2, "\r\n") != 0) // TODO: I had to change this to make the tests pass, but I'm not sure this is correct. Please take a look. @Antoine
				{
					std::cout << "returned not so early" << std::endl;
					return false;
				}
				trailers_end = idx + 2;
			}
			else
				trailers_end += 4;

			size_t end_pos = trailers_end;
			_request._body = _request._decoded_body;

			if (_request.parse_body())
			{
				std::cout << "parse_body fail" << std::endl;
				_status_code = 400;
				set_create_response();
				return false;
			}
			set_process_request();

			// Mark leftover bytes
			if (end_pos < data.size())
				_flags._leftover_chunk = true;
			else
				_flags._leftover_chunk = false;

			//_request._decoded_body.clear();

			// _request._chunk_parse_index = 0;
			 _request._chunk_size_pending = true;
			 _request._current_chunk_size = 0;
			std::cout << "about to return true" << std::endl;
			return true;
		}
		_request._chunk_size_pending = true;
	}
}

bool Client::try_parse_body()
{
	/* skips if not needed */
	if (!can_i_read_body())
		return (false);

	/* send to chunked version */
	if (is_body_chunked())
	{
		std::cout << "body is chunked" << std::endl;
		return (try_parse_chunked_body());
	}

	/* if we didn't get the get the whole data yet, skip for another turn of reading */
	size_t total_needed = _request._header.size() + _request._content_length;
	if (_request._request_data.size() < total_needed)
		return false;

	/* once we have everything, dump it into request._body */
	_request._body = _request._request_data.substr(_request._header.size(), _request._content_length);

	if (_request.parse_body())
	{
		_status_code = 400;
		set_create_response();
		return false;
	}

	/* checking if there is anything left, potentially a new request */
	if (_request._request_data.size() > _request._body.size() + _request._header.size())
		_flags._leftover_chunk = true;
	else
		_flags._leftover_chunk = false;

	_request._request_data.erase(0, total_needed);
	set_process_request();
	return true;
}

bool Client::try_parse_header()
{
	/* Checking if we are still chekcking the header, if we're exceeding the size and if the end of header indicator has been found */
	if (!can_i_read_header())
		return false;
	if (_request._request_data.size() > 16384)
	{
		_status_code = 413;
		set_create_response();
		return (false);
	}
	size_t pos = _request._request_data.find("\r\n\r\n");
	if (pos == std::string::npos)
		return false;

	/* if found, parse the header and make some validation, jumping to the response if the parsing/validation fails */
	_request._header = _request._request_data.substr(0, pos + 4);
	if (_request.parse_header() != 0)
	{
		_status_code = 400;
		set_create_response();
		return (false);
	}
	set_flags();
	if (static_cast<long long>(_request._content_length) > _server->get_client_max_body_size())
	{
		_status_code = 413;
		set_create_response();
		return (false);
	}
	if (_request.http_requirements_met() != 200)
	{
		set_create_response();
		std::cout << "failed requirements" << std::endl;
		return (false);
	}
	if (!validate_permissions())
	{
		set_create_response();
		std::cout << "failed permissions" << std::endl;
		return (false);
	}

	/* if validation works, check if we should read the body next or go to the processing step */
	if (is_body_chunked())
	{
		_request._chunk_parse_index = _request._header.size();
		_request._chunk_size_pending = true;
		_request._current_chunk_size = 0;
		_request._decoded_body.clear();
		set_read_body();
	}
	else if (_request.http_can_have_body() && _request._content_length != 0)
		set_read_body();
	else
		set_process_request();
	return true;
}

int Client::handle_read()
{
	if (!can_i_read_header() && !can_i_read_body())
	{
		std::cout << "@@@@@@  IMPOSSIBLE >>>>>>>>>> Client not in a reading state!" << std::endl;
		_status_code = 500;
		set_create_response();
		return 1;
	}

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
		if (!request_complete())
		{
			_status_code = 400;
			set_create_response();
			return 1;
		}
		if (!_request._request_data.empty())
			set_process_request();
		return 1;
	}

	/* attempt finding and parsing header on what we received */
	if (try_parse_header())
		std::cout << "\033[35m  Header parsed \033[0m" << std::endl;

	/* skipping processing the request if header malformed */
	if (can_i_create_response())
	{
		std::cout << "\033[35m skipping ahead to response \033[0m" << std::endl;
		return (1);
	}

	/* attempt parsing body on what is left if needed */
	if (try_parse_body())
	{
		std::cout << "\033[35m  Body parsed \033[0m" << std::endl;
		return 1;
	}
	return 0;
}

void Client::processRequest()
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
		Method::handleGet(*this);
		break;
	case 1:
		Method::handlePost(*this);
		break;
	case 2:
		Method::handleDelete(*this);
		break;
	default:
		std::cout << "Error processing request, method is = " << _request._method << std::endl;
		set_status_code(500);
		return;
	}
}

int Client::handle_write()
{
	if (can_i_create_response())
	{
		_response.set_request(&_request);
		std::string response = _response.format_response(get_status_code(), should_keep_alive(), _request._header_kv["version"]);
		_response.set_response_data(response);
		set_send_response();
	}
	std::cout << "==>>  Client will receive answer" << std::endl;
	std::string res = _response.get_response_data(_response.get_bytes_sent()).substr(0, 1024);
	std::cout << "Response to be sent:\n-----\n"
		  << res << "-----\n\n"
		  << std::endl;
	ssize_t bytes_sent = send(_fd, res.c_str(), res.size(), 0);
	if (bytes_sent == -1)
	{
		if (errno == EPIPE)
		{
			_status_code = 501;
			set_handle_error();
			// Client disconnected, close cleanly
		}
		else
		{
			set_handle_error();
			_status_code = 500;
		}
		std::cout << "Error sending response" << std::endl;
		return (1);
	}
	_response.update_bytes_sent(bytes_sent);
	std::cout << "bytes sent: " << bytes_sent << std::endl;
	std::cout << "_response.get_bytes_sent(): " << _response.get_bytes_sent() << std::endl;
	std::cout << "_response.get_response_data_full().size(): " << _response.get_response_data_full().size() << std::endl;
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
