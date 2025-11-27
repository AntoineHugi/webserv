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
		static bool	open_config_file(char *arg, Service& service);

		/* config file methods */ /* server */
		static bool	parse_config_file(std::vector<std::string>& tokens, Service &service);
		static bool	handle_server_index(Server &server, std::vector<std::string> &tokens, size_t &i);
		static bool	handle_server_allowance(Server &server, std::vector<std::string> &tokens, size_t &i);
		static bool	handle_server_error_page(Server &server, std::vector<std::string> &tokens, size_t &i);
		static bool	handle_server_keyval(Server &server, std::vector<std::string> &tokens, size_t &i);
		static bool	assign_single_keyval_server(Server& server, std::string& key, std::string& value);
		static bool	check_server(Server& server);
		static bool	missing_config(std::string missing);

		/* config file methods */ /* location */
		static bool	parse_location(Server* server, std::vector <std::string>& tokens, size_t& i);
		static bool	check_duplicates(Server *server, std::string route);
		static bool	handle_location_method(Route& route, std::vector <std::string>& tokens, size_t& i);
		static bool	handle_location_return(Route &route, std::vector<std::string> &tokens, size_t &i);
		static bool	handle_location_index(Route& route, std::vector <std::string>& tokens, size_t& i);
		static bool	handle_location_allowance(Route &route, std::vector<std::string> &tokens, size_t &i);
		static bool	handle_location_keyval(Route &route, std::vector<std::string> &tokens, size_t &i);
		static bool	assign_single_keyval_route(Route& route, std::string& key, std::string& value);
};

#endif
