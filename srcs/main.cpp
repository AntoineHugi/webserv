#include "./Functions/parserConfig.hpp"
#include "./Core/debugPrinting.hpp"

bool DEBUG = true;
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
		{
			print_red("Error: Failed to open config file", DEBUG);
			return (1);
		}
		else
		{
			while(i < service.servers.size())
			{
				service.servers[i].set_server();
				print_green("Server " + service.servers[i].get_name() +
				" configured on " + service.servers[i].get_host() +
				":" + convert_to_string(service.servers[i].get_port()) + " starting ...", DEBUG);
				i++;
			}
		}
		service.poll_service();
	}
	else
		std::cout << "please run the executable and 1 valid .conf file" << std::endl;
	return (0);
}
