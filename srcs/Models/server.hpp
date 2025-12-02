#ifndef SERVER_H
# define SERVER_H

# include "../Core/debugPrinting.hpp"

extern bool DEBUG;
extern const size_t BUFFER_SIZE;
extern int CLIENT_TIMEOUT_MS;


# include <string>
# include <cctype>
# include <stdio.h>
# include <cstdio>
# include <unistd.h>
# include <limits.h>
# include <cstring>
# include <iostream>
# include <vector>
# include <map>
# include <algorithm>
# include <fcntl.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <sys/wait.h>
# include <libgen.h>
# include <sstream>

# include "route.hpp"

class Server
{
	private:
		std::string _name;
		int _port;
		std::string _host;
		std::string _root;
		std::vector <std::string> _index;
		std::map <int, std::string> _error_page;
		std::map <std::string, std::string> _bouncer;
		unsigned long _client_max_body_size;
		int	_sock;
		std::vector <Route> _routes;

	public:
		Server();
		Server(const Server& other);
		Server& operator=(const Server& other);
		~Server();

		// Getters
		std::string	get_name() { return (_name); }
		int	get_port() { return (_port); }
		int	get_sock() { return (_sock); }
		std::string	get_host() { return (_host); }
		std::string	get_root() { return (_root); }
		std::vector <std::string>	get_index() { return (_index); }
		std::map <int, std::string>	get_error_page() { return (_error_page); }
		std::map <std::string, std::string>	get_bouncer() { return (_bouncer); }
		unsigned long	get_client_max_body_size() { return (_client_max_body_size); }
		std::vector <Route>	get_routes() { return (_routes); }

		// Setters
		void	set_name(const std::string& name) { _name = name; }
		void	set_sock(int sock) { _sock = sock; }
		void	set_host(const std::string& host) { _host = host; }
		void	set_root(const std::string& root) { _root = root; }
		void	set_index(const std::vector <std::string>& index) { _index = index; }
		void	add_error_page(int key, std::string& value) { _error_page.insert(std::make_pair(key, value)); }
		void	add_bouncer(std::string& key, std::string& value) { _bouncer.insert(std::make_pair(key, value)); }
		void	add_route(Route route) { _routes.push_back(route); }

		void	set_port(const std::string& port);
		void	set_client_max_body_size(const std::string& max);
		int		set_server();
};

#endif
