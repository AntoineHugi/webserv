#include "client.hpp"

Client::Client() : _state(READING_HEADERS),
				   _flags(),
				   _fd(-1),
				   _status_code(200),
				   _server(),
				   _request(),
				   _response() {}

Client::Client(int fd, Server &server) : _state(READING_HEADERS),
										 _flags(),
										 _fd(fd),
										 _status_code(200),
										 _server(&server),
										 _request(),
										 _response() {}

Client::Client(const Client &other) : _state(other._state),
									  _flags(other._flags),
									  _fd(other._fd),
									  _status_code(other._status_code),
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
		_server = other._server;
		_request = other._request;
		_response = other._response;
	}
	return *this;
}

Client::~Client() {}

int Client::return_set_status_code(int code)
{
	this->_status_code = code;
	return 1;
}

void Client::set_flags()
{
	if (this->_request._header_kv["Version"] == "HTTP/1.1") // HTTP/1.1 has keep-alive by default
	{
		this->_flags._should_keep_alive = true;
		if (this->_request._header_kv["Connection"] == "close")
			this->_flags._should_keep_alive = false;
	}
	else if (this->_request._header_kv["Version"] == "HTTP/1.0") // HTTP/1.0 requires explicit keep-alive
	{
		this->_flags._should_keep_alive = false;
		if (this->_request._header_kv["Connection"] == "keep-alive")
			this->_flags._should_keep_alive = true;
	}

	if (this->_request._header_kv["Transfer-Encoding"] == "chunked")
		this->_flags._body_chunked = true;
	else
		this->_flags._body_chunked = false;
}

void Client::refresh_client()
{
	this->_request.flush_request_data();
	this->_response.flush_response_data();
	this->_state = READING_HEADERS;
	this->_flags._should_keep_alive = false;
	this->_flags._body_chunked = false;
	// this->_flags._leftover_chunk = false;
	this->_status_code = 200;
}

/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/

bool Client::validate_permissions()
{
	/* looks the best matching route for that uri */
	std::vector<Route> routes = this->_server->get_routes();

	if (routes.empty())
	{
		std::cout << "routes are empty" << std::endl;
		Route default_route;
		std::vector<std::string> methods;
		methods.push_back("GET");
		default_route.set_path("/");
		default_route.set_methods(methods);
		routes.push_back(default_route);
	}
	int index = -1;
	std::string matchedPath;
	std::cout << "uri is " << _request._uri << std::endl;
	for (size_t i = 0; i < routes.size(); ++i)
	{
		std::string path = routes[i].get_path();
		std::cout << "path is " << path << std::endl;
		if (_request._uri.compare(0, path.size(), path) == 0)
		{
			if (path.size() >= matchedPath.size())
			{
				matchedPath = path;
				index = i;
			}
		}
	}
	std::cout << "matchedPath is " << matchedPath << std::endl;
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
		_request._root = this->_server->get_root();
		_request._fullPathURI = this->_server->get_root() + _request._uri.substr(matchedPath.size());
	}

	std::cout << "_fullPathURI is " << _request._fullPathURI << std::endl;
	/* checks that the target uri exists */
	if (stat(_request._fullPathURI.c_str(), &_request._stat) != 0)
	{
		set_status_code(404);
		return (false);
	}

	/* checks if it's a directory */
	if (S_ISDIR(_request._stat.st_mode))
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

int Client::handle_read()
{
	if (this->can_i_read_header() == false && this->can_i_read_body() == false)
	{
		std::cerr << "Client not in a reading state!" << std::endl;
		return (1);
	}
	std::cout << "\033[33mClient is sending data \033[0m" << std::endl;
	char buffer[80]; // char buffer[8192];
	int bytes_read = recv(this->_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes_read > 0)
		this->_request._request_data.append(buffer, bytes_read);

	if (bytes_read > 0 || this->leftover_chunk() == true)
	{
		this->_flags._leftover_chunk = false;
		if (this->can_i_read_header() == true)
		{
			std::cout << "\033[31m  Header not parsed \033[0m" << std::endl;
			if (this->_request._request_data.size() > 16384)
				return (return_set_status_code(431));
			// std::cout << "\n=====>>  Client request data so far:\n-----\n" << this->_request._request_data << "\n-----\n" << std::endl;
			if (this->_request._request_data.find("\r\n\r\n") != std::string::npos)
			{
				std::cout << "\033[35m  Header will be parsed \033[0m" << std::endl;
				this->_request._header = this->_request._request_data.substr(0, this->_request._request_data.find("\r\n\r\n") + 4);
				this->_request.parse_header();
				this->_status_code = this->_request.http_requirements_met();
				if (this->_status_code != 200)
					return (return_set_status_code(this->_status_code));
				if (this->_request._content_length > static_cast<size_t>(this->_server->get_client_max_body_size())) // e.g., 1 GB limit // TODO: use server config value
					return (return_set_status_code(431));
				if (this->_request.http_can_have_body())
					this->set_read_body();
				else
					this->set_process_request();
				if (!validate_permissions())
					return (1);
				this->set_flags();
				std::cout << "\033[35m  Header parsed \033[0m" << std::endl;
			}
		}

		if (this->can_i_read_body() == true && this->_request._content_length == 0)
		{
			if (this->_flags._body_chunked == true)
			{
				std::cout << "\033[35m  Chunked body handling not implemented \033[0m" << std::endl;
				if (this->_request._request_data.find("0\r\n\r\n") != std::string::npos)
				{
					this->_request._body = this->_request._request_data.substr(this->_request._header.size(), this->_request._request_data.find("0\r\n\r\n") + 5 - this->_request._header.size());
					if (this->_request._body.size() + this->_request._header.size() < this->_request._request_data.size())
						this->_flags._leftover_chunk = true;
					else
						this->_flags._leftover_chunk = false;

					this->_request.parse_body();
					this->set_process_request();
				}
			}
			else
				this->set_process_request();
		}
		if (this->can_i_read_body() == true && this->_request._content_length > 0)
		{
			if (this->_request._request_data.size() >= this->_request._header.size() + this->_request._content_length)
			{
				this->_request._body = this->_request._request_data.substr(this->_request._header.size(), this->_request._content_length);
				if (this->_request._body.size() + this->_request._header.size() < this->_request._request_data.size())
					this->_flags._leftover_chunk = true;
				else
					this->_flags._leftover_chunk = false;

				std::cout << "\033[35mBody: '" << this->_request._body << "'\n----\n"
						  << "\033[0m" << std::endl;
				this->_request.parse_body();
				this->set_process_request();
			}
		}
	}
	else if (bytes_read == 0)
	{
		std::cout << "===========>>  Client finished request" << std::endl;
		// status code to clarify
		return 1; // Signal to close
	}
	else
		return (return_set_status_code(500));

	std::cout << "Handle_read ==>>  Client content length: " << this->_request._content_length << std::endl;
	std::cout << "Handle_read ==>>  Client work request: " << this->get_state() << std::endl;
	return 0;
}

void Client::handle_write()
{
	std::cout << "==>>  Client will receive answer" << std::endl;

	// TODO: handle partial sends if size is within limits
	// TODO: if data to be send is too large, throw error
	std::string res = this->_response.get_response_data();
	std::cout << "Response to be sent:\n-----\n"
			  << res << "-----\n\n"
			  << std::endl;
	ssize_t bytes_sent = send(this->_fd, res.c_str(), strlen(res.c_str()), 0);
	if (bytes_sent == -1)
	{
		this->_status_code = 500;
		std::cerr << "Error sending response" << std::endl;
	}
}

void Client::processRequest()
{
	std::string methods[3] = {"GET", "POST", "DELETE"};
	int field = -1;
	for (int i = 0; i < 3; i++)
	{
		if (_request._header_kv["Method"] == methods[i])
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
		std::cout << "Error processing request" << std::endl;
		set_status_code(500);
		return;
	}
}

/*

		---

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
