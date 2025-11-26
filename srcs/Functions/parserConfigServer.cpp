#include "parserConfig.hpp"

bool Parser::assign_single_keyval_server(Server& server, std::string &key, std::string &value)
{
	std::string fields[5] = {"server_name", "listen", "host", "root", "client_max_body_size"};
	int field = -1;

	for (int i = 0; i < 5; i++)
	{
		if (key == fields[i])
			field = i;
	}
	switch (field)
	{
	case 0:
		if (!server.get_name().empty())
		{
			std::cout << "Config file error: duplicate server name" << std::endl;
			return (false);
		}
		server.set_name(value);
		break;
	case 1:
		if (server.get_port() != -1)
		{
			std::cout << "Config file error: duplicate port" << std::endl;
			return (false);
		}
		try
		{
			server.set_port(value);
		}
		catch (const std::exception &e)
		{
			std::cout << "Config file error: " << e.what() << std::endl;
			return (false);
		}
		break;
	case 2:
		if (!server.get_host().empty())
		{
			std::cout << "Config file error: duplicate host" << std::endl;
			return (false);
		}
		server.set_host(value);
		break;
	case 3:
		if (!server.get_root().empty())
		{
			std::cout << "Config file error: duplicate root" << std::endl;
			return (false);
		}
		server.set_root(value);
		break;
	case 4:
		if (server.get_client_max_body_size() != 0)
		{
			std::cout << "Config file error: duplicate max body size" << std::endl;
			return (false);
		}
		try { server.set_client_max_body_size(value); }
		catch (const std::exception &e)
		{
			std::cout << "Config file error: " << e.what() << std::endl;
			return (false);
		}
		break;
	default:
		std::cout << "Config file error: field not valid : " << key << std::endl;
		return (false);
	}
	return (true);
}

bool Parser::handle_server_keyval(Server &server, std::vector<std::string> &tokens, size_t &i)
{
	std::string key = tokens[i];
	++i;
	if (i >= tokens.size() || tokens[i] == ";")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	std::string value = tokens[i];
	++i;
	if (i >= tokens.size() || tokens[i] != ";")
	{
		--i;
		std::cout << "Config file error: expected ';' after " << tokens[i] << std::endl;
		return (false);
	}
	if (!assign_single_keyval_server(server, key, value))
		return (false);
	return (true);
}

bool Parser::handle_server_error_page(Server &server, std::vector<std::string> &tokens, size_t &i)
{
	++i;
	if (i >= tokens.size() || tokens[i] == ";" || tokens[i] == "{" || tokens[i] == "}")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	char *end;
	int key = static_cast<int>(std::strtol(tokens[i].c_str(), &end, 10));
	if (*end != '\0')
	{
		std::cout << "Config file error: invalid error code: " << tokens[i] << std::endl;
		return (false);
	}
	++i;
	if (i >= tokens.size() || tokens[i] == ";" || tokens[i] == "{" || tokens[i] == "}")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	std::string value = tokens[i];
	server.add_error_page(key, value);
	++i;
	if (i >= tokens.size() || tokens[i] != ";")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	return (true);
}

bool Parser::handle_server_allowance(Server &server, std::vector<std::string> &tokens, size_t &i)
{
	std::string key = tokens[i];
	++i;
	if (i >= tokens.size() || tokens[i] == ";" || tokens[i] == "{" || tokens[i] == "}")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	std::string value = tokens[i];
	++i;
	if (i >= tokens.size() || tokens[i] != ";")
	{
		--i;
		std::cout << "Config file error: expecting ';' after " << tokens[i] << std::endl;
		return (false);
	}
	server.add_bouncer(key, value);
	return (true);
}

bool Parser::handle_server_index(Server &server, std::vector<std::string> &tokens, size_t &i)
{
	++i;
	if (i >= tokens.size() || tokens[i] == ";" || tokens[i] == "{" || tokens[i] == "}")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	std::vector<std::string> index;
	index.push_back(tokens[i]);
	++i;
	while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "{" && tokens[i] != "}")
	{
		index.push_back(tokens[i]);
		++i;
	}
	if (i >= tokens.size() || tokens[i] != ";")
	{
		--i;
		std::cout << "Config file error: expecting ';' after " << tokens[i] << std::endl;
		return (false);
	}

	if (!server.get_index().empty())
	{
		std::cout << "Config file error: duplicate index line in server" << std::endl;
		return (false);
	}
	server.set_index(index);
	return (true);
}

bool Parser::parse_config_file(std::vector<std::string>& tokens, Service &service)
{
	size_t i = 0;

	while (i < tokens.size() && tokens[i] == "server")
	{
		++i;
		if (i >= tokens.size() || tokens[i] != "{")
		{
			--i;
			std::cout << "Config file error: expecting '{' after " << tokens[i] << std::endl;
			return (false);
		}
		++i;
		Server server;
		while (i < tokens.size() && tokens[i] != "}")
		{
			if (tokens[i] == ";" || tokens[i] == "{")
			{
				--i;
				std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
				return (false);
			}
			if (tokens[i] == "location")
			{
				if (!parse_location(&server, tokens, i))
					return (false);
			}
			else if (tokens[i] == "index")
			{
				if (!handle_server_index(server, tokens, i))
					return (false);
			}
			else if (tokens[i] == "allow" || tokens[i] == "deny")
			{
				if (!handle_server_allowance(server, tokens, i))
					return (false);
			}
			else if (tokens[i] == "error_page")
			{
				if (!handle_server_error_page(server, tokens, i))
					return (false);
			}
			else
			{
				if (!handle_server_keyval(server, tokens, i))
					return (false);
			}
			++i;
		}
		if (tokens[i] != "}")
		{
			--i;
			std::cout << "Config file error: expecting '}' after " << tokens[i] << std::endl;
			return (false);
		}
		++i;
		if (!check_server(server))
			return (false);
		service.servers.push_back(server);
	}
	if (i < tokens.size())
	{
		std::cout << "Config file error: unexpected token: " << tokens[i] << std::endl;
		return (false);
	}
	return (true);
}

std::vector<std::string> Parser::tokenise(std::string &str)
{
	std::vector<std::string> tokens;
	std::istringstream iss(str);
	std::string word;

	while (iss >> word)
	{
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

bool Parser::open_config_file(char *arg, Service &service)
{
	std::string filename = arg;
	std::string line;
	std::string config = "";

	if (filename.empty())
	{
		std::cout << "Config file error: no file name" << std::endl;
		return (false);
	}
	std::ifstream file(filename.c_str());
	if (file.is_open())
	{
		while (std::getline(file, line))
			config.append(line);
		file.close();
		if (config == "")
		{
			std::cout << "Config file error: file empty" << std::endl;
			return (false);
		}
		else
		{
			std::vector<std::string> tokens = Parser::tokenise(config);
			if (!parse_config_file(tokens, service))
				return (false);
			else
				return (true);
		}
	}
	else
	{
		std::cout << "Config file error: couldn't open file" << std::endl;
		return (false);
	}
}

bool Parser::missing_config(std::string missing)
{
	std::cout << "Config file error: " << missing << " missing" << std::endl;
	return (false);
}

bool Parser::check_server(Server &server)
{
	if (server.get_name().empty())
		return (missing_config("server name"));
	if (server.get_port() == -1)
		return (missing_config("port"));
	if (server.get_host().empty())
		return (missing_config("host"));
	if (server.get_root().empty())
		return (missing_config("root"));
	if (server.get_root()[server.get_root().size() - 1] != '/')
		server.set_root(server.get_root() + "/");
	if (server.get_index().empty())
		return (missing_config("index"));
	if (server.get_error_page().empty())
		return (missing_config("error page"));
	if (server.get_client_max_body_size() == 0)
		return (missing_config("max client body size"));
	if (server.get_routes().empty())
	{
		Route default_route;
		default_route.set_path("/");
		std::vector<std::string> methods;
		methods.push_back("GET");
		default_route.set_methods(methods);
		server.get_routes().push_back(default_route);
	}
	return (true);
}
