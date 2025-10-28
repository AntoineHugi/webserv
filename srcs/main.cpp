#include "parser.hpp"
#include "server.hpp"
#include "service.hpp"
#include <iostream>

int	main(int argc, char **argv)
{
	long unsigned int i = 0;
	if (argc == 2)
	{
		Service	service;
		if (!Parser::open_config_file(argv[1], &service))
			return (1);
		else
		{
			while(i < service.servers.size())
			{
				service.servers[i].set_server();
				std::cout << "Server: " << i << " starting... "<< std::endl;
				i++;
			}
			std::cout << service.servers[0].get_port() << std::endl;
			std::cout << service.servers[1].get_port() << std::endl;
			// opening up the sockets, binding etc
		}
		service.poll_service();
	}
	else
	{
		std::cout << "please run the executable and 1 valid .conf file" << std::endl;
	}
	return (0);
}
