#include "./Functions/parserConfig.hpp"
#include "./Core/debugPrinting.hpp"

bool DEBUG = false;
const size_t BUFFER_SIZE = 1048576;
int CLIENT_TIMEOUT_MS = 60000;

int	main(int argc, char **argv)
{
	long unsigned int i = 0;
	signal(SIGINT, handle_shutdown);
	signal(SIGTERM, handle_shutdown);
	signal(SIGPIPE, SIG_IGN);

	if (argc == 2)
	{
		Service	service;
		if (!Parser::open_config_file(argv[1], service))
			return (1);
		else
		{
			while(i < service.servers.size())
			{
				if (service.servers[i].set_server() == 1)
				{
					for (size_t j = 0; j < i; j++)
						close(service.servers[j].get_sock());
					print_red("Error in while seting up one server, please check config file", true);
					return (1);
				}
				std::ostringstream ss;
				ss << service.servers[i].get_port();
				std::string msg = "Server " + service.servers[i].get_name() + " configured on " + service.servers[i].get_host() + ":" + ss.str() + " starting ...";
				print_green(msg, DEBUG);
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
