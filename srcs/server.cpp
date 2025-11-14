#include "server.hpp"

Server::Server():_name(""), _port(-1), _host(""), _root(""), _index(), _error_page(""), _client_max_body_size(-1), _sock(-1) {}

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
	//TODO: Make deep copy of clients
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
std::string	Server::get_error_page() { return (this->_error_page); }
int	Server::get_client_max_body_size() { return (this->_client_max_body_size); }
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
void	Server::set_index(std::vector <std::string> index) { this->_index = index; }
void	Server::set_error_page(const std::string& page) { this->_error_page = page; }
void	Server::set_client_max_body_size(const std::string& max)
{
	for (size_t i = 0; i < max.size(); i++)
	{
		if (!std::isdigit(max[i]))
			throw (std::runtime_error("Invalid max client body size - not numerical value"));
	}
	this->_client_max_body_size = atoi(max.c_str());
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

bool	Server::validateRequest(Client& client)
{
	/* is this already done earlier? */
	if (client._request._content_length != sizeof(client._request._body))
	{
		client._status_code = 400;
		return;
	}
	std::string uri;
	std::map<std::string, std::string>::iterator mit = client._request._header_kv.find("URI");
	if (mit != client._request._header_kv.end()) 
		uri = mit->second;
	else
	{
		client._status_code = 500;
		return (false);
	}
	int index = -1;
	std::string matchedPath;
	std::vector<Route>::iterator vit = _routes.begin();
	for (size_t i = 0; i < this->_routes.size(); ++i)
	{
		if (uri.compare(0, this->_routes[i].get_path().size(), this->_routes[i].get_path()) == 0) 
		{
			if (this->_routes[i].get_path().size() > matchedPath.size())
			{
				matchedPath = this->_routes[i].get_path();
				index = i;
			}
		}
	}
	if (matchedPath.empty() || index == -1)
	{
		client._status_code = 404;
		return (false);
	}
	if (!this->_routes[index].get_root().empty())
	{
		client._request._root = this->_routes[index].get_root();
		client._request._fullPathURI = this->_routes[index].get_root() + uri.substr(matchedPath.size());
	}
	else
	{
		client._request._root = this->_root;
		client._request._fullPathURI = this->_root + uri.substr(matchedPath.size());
	}
	if (stat(client._request._fullPathURI.c_str(), &client._request._stat) != 0)
	{
		client._status_code = 404;
		return (false);
	}
	std::string method;
	std::map<std::string, std::string>::iterator it = client._request._header_kv.find("Method");
	if (it != client._request._header_kv.end()) 
		method = it->second;
	else
		return (false);
	if (S_ISDIR(client._request._stat.st_mode))
	{
		if (!this->_routes[index].get_autoindex().empty() && method == "GET" || method == "POST")
			client._request._isDirectory = true;
		else
		{
			client._status_code = 403;
			return false;
		}
	}
	for (size_t j = 0; j < this->_routes[index].get_methods().size(); ++j)
	{
		if (method == this->_routes[index].get_methods()[j])
			return (true);
	}
	client._status_code = 405;
	client._response._allowedMethods = this->_routes[index].get_methods();
	return (false);
}

void	Server::processRequest(Client& client)
{
	std::string methods[3] = {"GET", "POST", "DELETE"};
	int field = -1;
	for (int i = 0; i < 3; i++)
	{
		if (client._request._header_kv["Method"] == methods[i])
			field = i;
	}
	switch (field)
	{
		case 0:
			Method::handleGet(client);
			break;
		case 1:
			Method::handlePost(client);
			break;
		case 2:
			Method::handleDelete(client);
			break;
		default:
			std::cout << "Error processing request" << std::endl;
			client._status_code = 500;
			return;
	}
}


