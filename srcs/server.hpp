#ifndef SERVER_H
# define SERVER_H

# include <string>
# include <map>
# include <vector>
# include <algorithm>
# include "route.hpp"

class Server
{
	private:
		std::string _name;
		int _port;
		std::string _host;
		std::string _root;
		std::vector <std::string> _index;
		std::string _error_page;
		int _client_max_body_size;
		std::vector <Route> _routes;

	public:
		Server();
		Server(const Server& other);
		Server& operator=(const Server& other);
		~Server();

		void	set_name(std::string& name);
		void	set_port(int port);
		void	set_host(std::string& host);
		void	set_root(std::string& root);
		void	set_index(std::vector <std::string> index);
		void	set_error_page(std::string& page);
		void	set_client_max_body_size(int max);
		void	add_route(Route route);

		std::string	get_name();
		int	get_port();
		std::string	get_host();
		std::string	get_root();
		std::vector <std::string>	get_index();
		std::string	get_error_page();
		int	get_client_max_body_size();
		std::vector <Route>	get_routes();
};

#endif