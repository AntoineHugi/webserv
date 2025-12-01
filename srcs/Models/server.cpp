#include "server.hpp"

Server::Server():
	_name(""),
	_port(-1),
	_host(""),
	_root(""),
	_index(),
	_error_page(),
	_bouncer(),
	_client_max_body_size(0),
	_sock(-1),
	_routes()
{}

Server::Server(const Server& other) :
	_name(other._name),
	_port(other._port),
	_host(other._host),
	_root(other._root),
	_index(other._index),
	_error_page(other._error_page),
	_bouncer(other._bouncer),
	_client_max_body_size(other._client_max_body_size),
	_routes(other._routes)
{}

Server& Server::operator=(const Server& other)
{
	if (this != &other)
	{
		_name = other._name;
		_port = other._port;
		_host = other._host;
		_root = other._root;
		_index = other._index;
		_error_page = other._error_page;
		_bouncer = other._bouncer;
		_client_max_body_size = other._client_max_body_size;
		_routes = other._routes;
	}
	return (*this);
}

Server::~Server() {}

/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/


void	Server::set_port(const std::string& port)
{
	for (size_t i = 0; i < port.size(); i++)
	{
		if (!std::isdigit(port[i]))
			throw (std::runtime_error("Invalid port - not numerical value"));
	}
	_port = atoi(port.c_str());
}
void	Server::set_client_max_body_size(const std::string& max)
{
	if (max.empty())
		throw (std::runtime_error("Invalid max client body size - empty string"));
	errno = 0;
	char* last;
	unsigned long value = std::strtoul(max.c_str(), &last, 10);
	if (*last != '\0')
	{
		if (last[1] != '\0')
			throw (std::runtime_error("Invalid max client body size - invalid format"));
		if (last == max.c_str())
			throw std::runtime_error("Invalid max client body size - no digits");
		char suffix = std::tolower(*last);
		if (suffix == 'k')
			value *= 1024;
		else if (suffix == 'm')
			value *= 1024 * 1024;
		else if (suffix == 'g')
			value *= 1024 * 1024 * 1024;
		else
			throw (std::runtime_error("Invalid max client body size - wrong unit suffix"));
	}
	this->_client_max_body_size = value;
}

int Server::set_server(){

	set_sock(socket(AF_INET, SOCK_STREAM, 0));
	if (get_sock() < 0) {
		perror("Socket creation failed");
		return (1);
	}
	print_cyan("socket: " + convert_to_string(get_sock()), true);

	int optval = 1;
	int check = setsockopt(get_sock(), SOL_SOCKET , SO_REUSEADDR , &optval, sizeof(optval));
	if (check < 0) {
		perror("Socket option failed");
		return (1);
	}
	int flags = fcntl(get_sock(), F_GETFL);
	check = fcntl(get_sock(), F_SETFL, flags | O_NONBLOCK);
	if (check < 0) {
		perror("Socket flags failed");
		return (1);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(get_port());
	check = bind(get_sock(), (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (check < 0) {
		perror("Bind failed");
		return (1);
	}
	check = listen(get_sock(), 128);
	if (check < 0) {
		perror("Listen failed");
		return (1);
	}
	print_cyan("Server listening on port: " + convert_to_string(get_port()), true);
	return (0);
}
