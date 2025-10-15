#ifndef SERVER_H
# define SERVER_H

# include <string>

class Server
{
	private:
		std::string _name;
		std::string _port;
		std::string _host;
		std::string _root;
		std::string _index;
		std::string _error_page;
	
	public:
		Server();
		Server(const Server& other);
		Server& operator=(const Server& other);
		~Server();
};

#endif