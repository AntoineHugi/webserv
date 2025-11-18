#ifndef SERVER_H
# define SERVER_H

# include <string>
# include <cctype>
# include <stdio.h>
# include <cstdio>
# include <unistd.h>
# include <limits.h>
# include <errno.h>
# include <cstring>
# include <iostream>
# include <vector>
# include <map>
# include <algorithm>
# include <fcntl.h>
# include <netinet/in.h>
# include <sys/socket.h>
# include <sys/wait.h>
# include <libgen.h>
// # include "client.hpp"
// # include "../Functions/method.hpp"
# include "route.hpp"

class Server
{
	private:
		std::string _name;
		int _port;
		std::string _host;
		std::string _root;
		std::vector <std::string> _index;
		std::vector <std::string> _error_page;
		long long _client_max_body_size;
		int	_sock;
		std::vector <Route> _routes;

	public:
		Server();
		Server(const Server& other);
		Server& operator=(const Server& other);
		~Server();

		// Getters
		std::string	get_name();
		int	get_port();
		int	get_sock();
		std::string	get_host();
		std::string	get_root();
		std::vector <std::string>	get_index();
		std::vector <std::string>	get_error_page();
		long long	get_client_max_body_size();
		std::vector <Route>	get_routes();

		// Setters
		void	set_name(const std::string& name);
		void	set_port(const std::string& port);
		void	set_sock(int sock);
		void	set_host(const std::string& host);
		void	set_root(const std::string& root);
		void	set_index(const std::vector <std::string>& index);
		void	set_error_page(const std::vector <std::string>& page);
		void	set_client_max_body_size(const std::string& max);
		void	add_route(Route route);

		void	set_server();
};

#endif
