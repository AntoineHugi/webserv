#ifndef PARSER_H
# define PARSER_H

# include "service.hpp"
# include "server.hpp"
# include <iostream>
# include <fstream>
# include <string>
# include <algorithm>
# include <vector>
# include <sstream>

class Parser
{	
	public:
		static bool	open_config_file(char *arg, Service* service);
		static bool	parse_file(std::string config_str, Service* service);
		static std::vector<std::string> tokenise(std::string& str);
		static bool assign_keyval(Server* server, std::string& key, std::string& value);
		static bool	check_server(Server* server);
};

#endif