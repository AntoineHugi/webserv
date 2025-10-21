#ifndef PARSER_H
# define PARSER_H

# include "service.hpp"
# include "server.hpp"
# include <iostream>
# include <fstream>
# include <string>
# include <cctype>
# include <algorithm>
# include <vector>
# include <sstream>

class Parser
{	
	public:

		/* general */
		static std::vector<std::string>	tokenise(std::string& str);

		/* config file methods */
		static bool	open_config_file(char *arg, Service* service);
		static bool	parse_config_file(std::string config_str, Service* service);
		static bool	parse_location(Server* server, std::vector <std::string> tokens, size_t* i);
		static bool	assign_single_keyval(Server* server, std::string& key, std::string& value);
		static bool	assign_vector_keyval(Server* server, std::string& key, std::vector <std::string> values);
		static bool	check_server(Server* server);

		/* client request methods */

};

#endif