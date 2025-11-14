#include "client.hpp"

Client::Client():
	_state(READING_HEADERS),
	_flags(),
	_fd(-1),
	_server(),
	_status_code(200),
	_request(),
	_response() {}


Client::Client(int fd, Server& server):
	_state(READING_HEADERS),
	_flags(),
	_fd(fd),
	_server(&server),
	_status_code(200),
	_request(),
	_response() {}

Client::Client(const Client& other) :
	_state(other._state),
	_flags(other._flags),
	_fd(other._fd),
	_server(other._server),
	_status_code(other._status_code),
	_request(other._request),
	_response(other._response) {}

Client& Client::operator=(const Client& other)
{
	if (this != &other)
	{
		_state = other._state;
		_flags = other._flags;
		_fd = other._fd;
		_server = other._server;
		_status_code = other._status_code;
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
	else if (this->_request._header_kv["Version"] == "HTTP/1.0") 	// HTTP/1.0 requires explicit keep-alive
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

int	Client::handle_read()
{
	if(this->can_i_read_header() == false && this->can_i_read_body() == false)
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
			std::cout << "\033[31m  Header not parsed \033[0m"<<std::endl;
			if (this->_request._request_data.size() > 16384)
				return(return_set_status_code(431));
			// std::cout << "\n=====>>  Client request data so far:\n-----\n" << this->_request._request_data << "\n-----\n" << std::endl;
			if(this->_request._request_data.find("\r\n\r\n") != std::string::npos)
			{
				std::cout << "\033[35m  Header will be parsed \033[0m"<<std::endl;
				this->_request._header = this->_request._request_data.substr(0, this->_request._request_data.find("\r\n\r\n") + 4);
				this->_request.parse_header();
				this->set_flags();
				this->_status_code = this->_request.http_requirements_met();
				if (this->_status_code != 200)
					return(return_set_status_code(this->_status_code));
				if (this->_request._content_length > 400)  // e.g., 1 GB limit // TODO: use server config value
					return(return_set_status_code(431));
				if (this->_request.http_can_have_body())
					this->set_read_body();
				else
					this->set_process_request();
				std::cout << "\033[35m  Header parsed \033[0m"<<std::endl;
			}
		}

		if(this->can_i_read_body() == true && this->_request._content_length == 0)
		{
			if (this->_flags._body_chunked == true)
			{
				std::cout << "\033[35m  Chunked body handling not implemented \033[0m"<<std::endl;
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
		if(this->can_i_read_body() == true && this->_request._content_length > 0)
		{
			if (this->_request._request_data.size() >= this->_request._header.size() + this->_request._content_length)
			{
				this->_request._body = this->_request._request_data.substr(this->_request._header.size(), this->_request._content_length);
				if (this->_request._body.size() + this->_request._header.size() < this->_request._request_data.size())
					this->_flags._leftover_chunk = true;
				else
					this->_flags._leftover_chunk = false;

				std::cout << "\033[35mBody: '" << this->_request._body << "'\n----\n" << "\033[0m"<<std::endl;
				this->_request.parse_body();
				this->set_process_request();
			}
		}
	}
	else if (bytes_read == 0)
	{
			std::cout << "===========>>  Client finished request" << std::endl;
			// status code to clarify
			return 1;  // Signal to close
	}
	else
		return(return_set_status_code(500));

	std::cout << "Handle_read ==>>  Client content length: " << this->_request._content_length <<  std::endl;
	std::cout << "Handle_read ==>>  Client work request: " << this->get_state() <<  std::endl;
	return 0;
}


void	Client::handle_write()
{
	std::cout << "==>>  Client will receive answer" << std::endl;

	// TODO: handle partial sends if size is within limits
	// TODO: if data to be send is too large, throw error
	std::string res = this->_response.get_response_data();
	std::cout << "Response to be sent:\n-----\n" << res << "-----\n\n" << std::endl;
	ssize_t bytes_sent = send(this->_fd, res.c_str(), strlen(res.c_str()), 0);
	if (bytes_sent == -1)
	{
		this->_status_code = 500;
		std::cerr << "Error sending response" << std::endl;
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
