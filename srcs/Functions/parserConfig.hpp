#ifndef PARSER_H
# define PARSER_H

# include "../Core/service.hpp"
# include "../Models/server.hpp"
# include <iostream>
# include <fstream>
# include <sstream>

class Parser
{
	public:

		/* general */
		static std::vector<std::string>	tokenise(std::string& str);

		/* config file methods */ /* server */
		static bool	open_config_file(char *arg, Service* service);
		static bool	parse_config_file(std::string config_str, Service* service);
		static bool	parse_location(Server* server, std::vector <std::string> tokens, size_t* i);
		static bool	assign_single_keyval_server(Server* server, std::string& key, std::string& value);
		static bool	assign_vector_keyval_server(Server* server, std::string& key, std::vector <std::string> values);
		static bool	check_server(Server* server);

		/* config file methods */ /* route */
		static bool	assign_single_keyval_route(Route* route, std::string& key, std::string& value);
		static bool	assign_vector_keyval_route(Route* route, std::string& key, std::vector <std::string> values);

		/* client request methods */

};

#endif
