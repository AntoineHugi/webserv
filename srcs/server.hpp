#ifndef SERVER_H
# define SERVER_H

# include <string>
# include <vector>
# include <cctype>
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
		int	_sock;
		std::vector<int> _clients;
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
		std::string	get_error_page();
		int	get_client_max_body_size();
		std::vector <Route>	get_routes();

		// // Setters
		void	set_name(const std::string& name);
		void	set_port(const std::string& port);
		void	set_sock(int sock);
		void	set_host(const std::string& host);
		void	set_root(const std::string& root);
		void	set_index(const std::vector <std::string> index);
		void	set_error_page(const std::string& page);
		void	set_client_max_body_size(const std::string& max);
		void	add_route(Route route);

		void	set_server();

		void create_listening_socket();
		void accept_new_client();
		// void handle_client_read(Client& client);
		// void handle_client_write(Client& client);



};

#endif
