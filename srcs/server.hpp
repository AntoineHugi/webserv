#ifndef SERVER_H
# define SERVER_H

# include <string>
# include <vector>

class Server
{
	private:
		std::string _name;
		int _port;
		std::string _host;
		std::string _root;
		std::string _index;
		std::string _error_page;
		int	_client_max_body_size;
		int	_sock;
		std::vector<int> _clients;
	
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
		std::string	get_index();
		std::string	get_error_page();
		int	get_client_max_body_size();

		// // Setters
		void	set_name(std::string name);
		void	set_port(int port);
		void	set_sock(int sock);
		void	set_host(std::string host);
		void	set_root(std::string root);
		void	set_index(std::string index);
		void	set_error_page(std::string page);
		void	set_client_max_body_size(int max);
		
		void	set_server();

		void create_listening_socket();
		void accept_new_client();
		// void handle_client_read(Client& client);
		// void handle_client_write(Client& client);


};

#endif