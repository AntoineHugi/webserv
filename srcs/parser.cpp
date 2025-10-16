#include "parser.hpp"

bool	Parser::open_config_file(char *arg, Service* service)
{
	std::string filename = arg;
	std::string	line;
	std::string	config = "";

	if (filename.empty())
	{
		std::cout << "Error: no file name" << std::endl;
		return (false);
	}
	std::ifstream file(filename.c_str());
	if (file.is_open())
	{
		while (std::getline(file, line))
			config.append(line);
		file.close();
		if (config == "")
			return (false);
		else
		{
			if (!parse_file(config, service))
				return (false);
			else
				return (true);
		}
	}
	else
	{
		std::cout << "Error: couldn't open file" << std::endl;
		return (false);
	}
}

bool	Parser::parse_file(std::string config, Service* service)
{
	std::vector <std::string> tokens = Parser::tokenise(config);

	for (size_t i = 0; i < tokens.size(); i++)
	{
		if (tokens[i] == "server")
		{
			++i;
			if (i >= tokens.size() || tokens[i] != "{")
				return (false);
			++i;
			Server server;
			while (i < tokens.size() && tokens[i] != "}")
			{
				std::string key = tokens[i];
				++i;
				if (tokens[i] == ";")
					return (false);
				std::string value = tokens[i];
				++i;
				if (tokens[i] != ";")
					return (false);
				if (!Parser::assign_keyval(&server, key, value))
					return (false);
				++i;
			}
			if (tokens[i] != "}")
				return (false);
			++i;
			if (!Parser::check_server(&server))
				return (false);
			service->servers.push_back(server);
		}
	}
	return (true);
}

std::vector<std::string> Parser::tokenise(std::string& str)
{
	std::vector<std::string> tokens;
	std::istringstream iss(str);
	std::string word;

	while (iss >> word) {
		std::string current;

		for (size_t i = 0; i < word.size(); ++i)
		{
			if (word[i] == '{' || word[i] == '}' || word[i] == ';') 
			{
				if (!current.empty()) 
				{
					tokens.push_back(current);
					current.clear();
				}
				tokens.push_back(std::string(1, word[i]));
			}
			else
				current += word[i];
		}
		if (!current.empty())
			tokens.push_back(current);
	}
	return (tokens);
}

bool Parser::assign_keyval(Server* server, std::string& key, std::string& value)
{
	std::string fields[7] = {"server_name", "listen", "host", "root", "index", "error_page", "client_max_body_size"};
	int field = -1;

	for (int i = 0; i < 7; i++)
	{
		if (key == fields[i])
			field = i;
	}
	switch (field)
	{
		case 0:
			if (!server->get_name().empty())
				return (false);
			server->set_name(value);
			break ;
		case 1:
			if (server->get_port() != -1)
				return (false);
			server->set_port(atoi(value.c_str()));
			break ;
		case 2:
			if (!server->get_host().empty())
				return (false);
			server->set_host(value);
			break ;
		case 3:
			if (!server->get_root().empty())
				return (false);
			server->set_root(value);
			break ;
		case 4:
			if (!server->get_index().empty())
				return (false);
			server->set_index(value);
			break ;
		case 5:
			if (!server->get_error_page().empty())
				return (false);
			server->set_error_page(value);
			break ;
		case 6:
			if (server->get_client_max_body_size() != -1)
				return (false);
			server->set_client_max_body_size(atoi(value.c_str()));
			break;
		default:
			return (false);
	}
	return (true);
}

bool Parser::check_server(Server* server)
{
	if (server->get_name().empty())
		return (false);
	if (server->get_port() == -1)
		return (false);
	if (server->get_host().empty())
		return (false);
	if (server->get_root().empty())
		return (false);
	if (server->get_index().empty())
		return (false);
	if (server->get_error_page().empty())
		return (false);
	if (server->get_client_max_body_size() == -1)
		return (false);
	return (true);
}