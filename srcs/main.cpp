#include "./Functions/parserConfig.hpp"
#include "./Models/server.hpp"
#include "./Core/service.hpp"
#include <iostream>

int	main(int argc, char **argv)
{
	long unsigned int i = 0;
	signal(SIGINT, handle_shutdown);
	signal(SIGTERM, handle_shutdown);
	signal(SIGPIPE, SIG_IGN);

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
		}
		service.poll_service();
	}
	else
		std::cout << "please run the executable and 1 valid .conf file" << std::endl;
	return (0);
}

// TODO: Implement proper logging flag, so we see debug info only when needed
// TODO: Implement signal handling for graceful shutdown
// TODO: clear memory and open FDs when exiting
// TODO: check strings case nonsensitivity where applicable (e.g., header fields)
// TODO: test malformed requests and edge cases
