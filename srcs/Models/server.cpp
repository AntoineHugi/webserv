#include "server.hpp"

Server::Server():
	_name(""), 
	_port(-1), 
	_host(""), 
	_root(""), 
	_index(), 
	_error_page(),
	_bouncer(),
	_client_max_body_size(-1), 
	_sock(-1),
	_routes()
	{}

Server::~Server() {}

Server::Server(const Server& other)
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

std::string	Server::get_name() { return (_name); }
int	Server::get_port() { return (_port); }
int	Server::get_sock() { return (_sock); }
std::string	Server::get_host() { return (_host); }
std::string	Server::get_root() { return (_root); }
std::vector <std::string>	Server::get_index() { return (_index); }
std::map <std::string, std::string>	Server::get_error_page() { return (_error_page); }
std::map <std::string, std::string>	Server::get_bouncer() { return (_bouncer); }
long long	Server::get_client_max_body_size() { return (_client_max_body_size); }
std::vector <Route>	Server::get_routes() { return (_routes); }


void	Server::set_name(const std::string& name) { _name = name; }
void	Server::set_port(const std::string& port)
{
	for (size_t i = 0; i < port.size(); i++)
	{
		if (!std::isdigit(port[i]))
			throw (std::runtime_error("Invalid port - not numerical value"));
	}
	_port = atoi(port.c_str());
}
void	Server::set_sock(int sock) { _sock = sock; }
void	Server::set_host(const std::string& host) { _host = host; }
void	Server::set_root(const std::string& root) { _root = root; }
void	Server::set_index(const std::vector <std::string>& index) { _index = index; }
void	Server::set_error_page(std::string& key, std::string& value) { _error_page.insert(std::make_pair(key, value)); }
void	Server::set_bouncer(std::string& key, std::string& value) { _bouncer.insert(std::make_pair(key, value)); }
void	Server::set_client_max_body_size(const std::string& max) 
{
	if (max.empty())
		throw (std::runtime_error("Invalid max client body size - empty string"));
	errno = 0;
	char* last;
	long long value = std::strtoll(max.c_str(), &last, 10);
	if (errno == ERANGE)
		throw (std::runtime_error("Invalid max client body size - out of range"));
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
void	Server::add_route(Route route) { _routes.push_back(route); }

void Server::set_server(){

	set_sock(socket(AF_INET, SOCK_STREAM, 0));
	if (get_sock() < 0) {
		perror("Socket creation failed");
		return; // TODO: throw correct error
	}
	std::cout << "socket: " << get_sock() << std::endl;

	int optval = 1;
	int check = setsockopt(get_sock(), SOL_SOCKET , SO_REUSEADDR , &optval, sizeof(optval));
	if (check < 0) {
		perror("Socket option failed");
		return; // TODO: throw correct error
	}
	int flags = fcntl(get_sock(), F_GETFL); // Get current flags
	check = fcntl(get_sock(), F_SETFL, flags | O_NONBLOCK); // Set non-blocking flag
	// Interesting for CGI
	// fcntl(fd, F_SETFD, FD_CLOEXEC); // Set close-on-exec flag
	if (check < 0) {
		perror("Socket flags failed");
		return; // TODO: throw correct error
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET; // IPv4
	server_addr.sin_addr.s_addr = INADDR_ANY; // Any interface
	server_addr.sin_port = htons(get_port()); // Port 8080
	check = bind(get_sock(), (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (check < 0) {
		perror("Bind failed");
		return; // TODO: throw correct error
	}
	check = listen(get_sock(), 128);
	if (check < 0) {
		perror("Listen failed");
		return; // TODO: throw correct error
	}
	std::cout << "Server listening on port " << get_port() << std::endl;
}

