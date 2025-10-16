#ifndef SERVER_H
# define SERVER_H

# include <string>

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
	
	public:
		Server();
		Server(const Server& other);
		Server& operator=(const Server& other);
		~Server();
		void	set_name(std::string name);
		std::string	get_name();
		void	set_port(int port);
		int	get_port();
		void	set_host(std::string host);
		std::string	get_host();
		void	set_root(std::string root);
		std::string	get_root();
		void	set_index(std::string index);
		std::string	get_index();
		void	set_error_page(std::string page);
		std::string	get_error_page();
		void	set_client_max_body_size(int max);
		int	get_client_max_body_size();
};

#endif