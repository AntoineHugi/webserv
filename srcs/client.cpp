#include "client.hpp"

Client::Client() {}

Client::Client(const Client& other) 
{
	_request = other._request;
	_response = other._response;
	_header_parsed = other._header_parsed;
	_work_request = other._work_request;
	_status_code = other._status_code;
	_inORout = other._inORout;
	_should_keep_alive = other._should_keep_alive;
	_server = other._server;
}

Client& Client::operator=(const Client& other)
{
	if (this != &other)
	{
		_request = other._request;
		_response = other._response;
		_header_parsed = other._header_parsed;
		_work_request = other._work_request;
		_status_code = other._status_code;
		_inORout = other._inORout;
		_should_keep_alive = other._should_keep_alive;
		_server = other._server;
	}
	return *this;
}

Client::~Client() {}

int fill_request_data(int fd, Client& client)
{
	std::cout << "\033[33mClient is sending data \033[0m" << std::endl;
	char buffer[80]; // char buffer[8192];
	int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes_read > 0)
	{
		client._request._request_data.append(buffer, bytes_read);
		if (!client._header_parsed)
		{
			std::cout << "\033[31m  Header not parsed \033[0m"<<std::endl;
			if (client._request._request_data.size() > 16384)
			{
				client._status_code = 431;
				return 1;
			}

			if(client._request._request_data.find("\r\n\r\n") != std::string::npos)
			{
				client._request._header = client._request._request_data.substr(0, client._request._request_data.find("\r\n\r\n") + 4);
				client.parse_header();
				std::cout << "\033[35m  Header parsed \033[0m"<<std::endl;
				if (client._request._content_length > 400)  // e.g., 1 GB limit
				{
					client._status_code = 431;
					return 1;
				}
			}
		}

		if(client._header_parsed == true && client._request._content_length == 0)
		{
			std::cout << "\033[35m  work request is true \033[0m"<<std::endl;
			client._work_request = true;
		}
		if(client._header_parsed == true && client._request._content_length > 0)
		{
			if (client._request._request_data.size() >= client._request._header.size() + client._request._content_length)
			{
				client._request._body = client._request._request_data.substr(client._request._header.size(), client._request._content_length);
				std::cout << "\033[35mBody: '" << client._request._body << "----\n" << "'\033[0m"<<std::endl;
				client.parse_body();
			}
		}
	}
	else if (bytes_read == 0)
	{
			std::cout << "===========>>  Client will send request" << std::endl;
			// status code to clarify
			return 1;  // Signal to close
	}
	else
	{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
					return 0;  // No data available, try again later

			// Actual error
			std::cerr << "recv error: " << strerror(errno) << std::endl;
			client._status_code = 500;
			return 1;  // Signal to close
	}
	std::cout << "===========>>  Client content length" << client._request._content_length <<  std::endl;
	std::cout << "===========>>  Client work request" << client._work_request <<  std::endl;
	return 0;
}


int	Client::handle_read()
{
	if (this->_inORout == true)
	{
		std::cout << "==>>  Client is waiting for answer" << std::endl;
		return (1);
	}
	if (fill_request_data(this->_fd, *this))
		return (1);
	return (0);
}

void	Client::handle_write()
{
	std::cout << "==>>  Client will receive answer" << std::endl;

	std::string res = this->_response._response_data;
	std::cout << "Response to be sent:\n-----\n" << res << std::endl;

	ssize_t bytes_sent = send(this->_fd, res.c_str(), strlen(res.c_str()), 0);
	if (bytes_sent == -1) 
	{
		this->_status_code = 500;
		std::cerr << "Error sending response" << std::endl;
	}
}

void Client::parse_header()
{
	std::cout << "\033[34mParsing header...\n\033[0m" << std::endl;
	std::cout << this->_request._request_data << std::endl;
	std::string line;
	std::istringstream stream(this->_request._header);

	std::getline(stream, line);
	size_t first_space = line.find(" ");
	size_t second_space = line.find(" ", first_space + 1);

	if (first_space != std::string::npos && second_space != std::string::npos) {
			this->_request._header_kv["Method"] = line.substr(0, first_space);
			this->_request._header_kv["URI"] = line.substr(first_space + 1, second_space - first_space - 1);
			this->_request._header_kv["Version"] = line.substr(second_space + 1);
			if (!this->_request._header_kv["Version"].empty() && this->_request._header_kv["Version"].substr(this->_request._header_kv["Version"].size()-1) == "\r")
				this->_request._header_kv["Version"].erase(this->_request._header_kv["Version"].size() - 1);
		}

	while (std::getline(stream, line)) {
		size_t colon_pos = line.find(":");
		if (colon_pos != std::string::npos) {
				std::string key = line.substr(0, colon_pos);
				key.erase(key.find_last_not_of(" ") + 1);
				std::string value = line.substr(colon_pos + 1, line.size() - 1);
				value.erase(0, value.find_first_not_of(" "));
				if (!value.empty() && value.substr(value.size()-1) == "\r")
					value.erase(value.size() - 1);
				this->_request._header_kv[key] = value;
		}
	}
	this->_header_parsed = true;
	this->_request._content_length = this->_request._header_kv["Content-Length"].empty() ? 0 : std::atoi(this->_request._header_kv["Content-Length"].c_str());

	if (this->_request._header_kv["Version"] == "HTTP/1.1") // HTTP/1.1 has keep-alive by default
	{
		this->_should_keep_alive = true;
		if (this->_request._header_kv["Connection"] == "close")
			this->_should_keep_alive = false;
	}
	else if (this->_request._header_kv["Version"] == "HTTP/1.0") 	// HTTP/1.0 requires explicit keep-alive
	{
		this->_should_keep_alive = false;
		if (this->_request._header_kv["Connection"] == "keep-alive")
			this->_should_keep_alive = true;
	}

	std::map<std::string, std::string>::iterator it;
	for (it = this->_request._header_kv.begin(); it != this->_request._header_kv.end(); ++it) {
			std::cout << "'" << it->first << "' : '" << it->second << "'" << std::endl;
	}
}

void Client::parse_body()
{
	/* WIP */
	this->_work_request = true;
}


std::string format_response(Client &client)
{
	std::string response;
	std::ostringstream ss;
	client._status_code = 200;
	ss << client._status_code;

	response += "HTTP/1.1 " + ss.str() + " ";
	switch (client._status_code) {
		case 200:
			response += "OK\r\n";
			break;
		case 400:
			response += "Bad Request\r\n";
			break;
		case 404:
			response += "Not Found\r\n";
			break;
		case 431:
			response += "Request Header Fields Too Large\r\n";
			break;
		default:
			response += "Internal Server Error\r\n";
			break;
	}

	response += "Content-Length:0";
	//response += _content_length; response length based on request. (maybe simply request_result.size())
	response += "\r\n";

	if (client._should_keep_alive)
			response += "Connection: keep-alive\r\n";
	else
			response += "Connection: close\r\n";
	response += "\r\n";

	// insert body

	return response;
}