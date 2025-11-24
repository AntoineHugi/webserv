#include "server.hpp"

Server::Server():
	_name(""), 
	_port(-1), 
	_host(""), 
	_root(""), 
	_index(), 
	_error_page(),
	_client_max_body_size(-1), 
	_sock(-1),
	_routes()
	{}

Server::~Server() {}

Server::Server(const Server& other)
{
	this->_name = other._name;
	this->_port = other._port;
	this->_host = other._host;
	this->_root = other._root;
	this->_index = other._index;
	this->_error_page = other._error_page;
	this->_client_max_body_size = other._client_max_body_size;
	this->_routes = other._routes;
}

Server& Server::operator=(const Server& other)
{
	if (this != &other)
	{
		this->_name = other._name;
		this->_port = other._port;
		this->_host = other._host;
		this->_root = other._root;
		this->_index = other._index;
		this->_error_page = other._error_page;
		this->_client_max_body_size = other._client_max_body_size;
		this->_routes = other._routes;
	}
	return (*this);
}

std::string	Server::get_name() { return (this->_name); }
int	Server::get_port() { return (this->_port); }
int	Server::get_sock() { return (this->_sock); }
std::string	Server::get_host() { return (this->_host); }
std::string	Server::get_root() { return (this->_root); }
std::vector <std::string>	Server::get_index() { return (this->_index); }
std::map <std::string, std::string>	Server::get_error_page() { return (this->_error_page); }
long long	Server::get_client_max_body_size() { return (this->_client_max_body_size); }
std::vector <Route>	Server::get_routes() { return (this->_routes); }


void	Server::set_name(const std::string& name) { this->_name = name; }
void	Server::set_port(const std::string& port)
{
	for (size_t i = 0; i < port.size(); i++)
	{
		if (!std::isdigit(port[i]))
			throw (std::runtime_error("Invalid port - not numerical value"));
	}
	this->_port = atoi(port.c_str());
}
void	Server::set_sock(int sock) { this->_sock = sock; }
void	Server::set_host(const std::string& host) { this->_host = host; }
void	Server::set_root(const std::string& root) { this->_root = root; }
void	Server::set_index(const std::vector <std::string>& index) { this->_index = index; }
void	Server::set_error_page(std::string& key, std::string& value) { this->_error_page.insert(std::make_pair(key, value)); }
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
void	Server::add_route(Route route) { this->_routes.push_back(route); }

void Server::set_server(){

	this->set_sock(socket(AF_INET, SOCK_STREAM, 0));
	if (this->get_sock() < 0) {
		perror("Socket creation failed");
		return; // TODO: throw correct error
	}
	std::cout << "socket: " << this->get_sock() << std::endl;

	int optval = 1;
	int check = setsockopt(this->get_sock(), SOL_SOCKET , SO_REUSEADDR , &optval, sizeof(optval));
	if (check < 0) {
		perror("Socket option failed");
		return; // TODO: throw correct error
	}
	int flags = fcntl(this->get_sock(), F_GETFL); // Get current flags
	check = fcntl(this->get_sock(), F_SETFL, flags | O_NONBLOCK); // Set non-blocking flag
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
	server_addr.sin_port = htons(this->get_port()); // Port 8080
	check = bind(this->get_sock(), (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (check < 0) {
		perror("Bind failed");
		return; // TODO: throw correct error
	}
	check = listen(this->get_sock(), 128);
	if (check < 0) {
		perror("Listen failed");
		return; // TODO: throw correct error
	}
	std::cout << "Server listening on port " << this->get_port() << std::endl;
}

