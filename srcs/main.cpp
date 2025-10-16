#include "parser.hpp"
#include "server.hpp"
#include "service.hpp"
#include <iostream>

int	main(int argc, char **argv)
{
	if (argc == 2)
	{
		Service	service;
		if (!Parser::open_config_file(argv[1], &service))
			std::cout << "config file error" << std::endl;
		else
		{
			std::cout << service.servers[0].get_name() << std::endl;
			// oepning up the sockets, binding etc
		}

	}
	else
	{
		std::cout << "please run the executable and 1 valid .conf file" << std::endl;
	}
	return (0);
}