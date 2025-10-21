#include "parser.hpp"

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

bool	Parser::open_config_file(char *arg, Service* service)
{
	std::string filename = arg;
	std::string	line;
	std::string	config = "";

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
			if (!parse_config_file(config, service))
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

bool	Parser::parse_config_file(std::string config, Service* service)
{
	std::vector <std::string> tokens = Parser::tokenise(config);
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
			if (tokens[i] == "location")
			{
				++i;
				if (i >= tokens.size() || tokens[i] == ";")
				{
					--i;
					std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
					return (false);
				}
				if (!parse_location(&server, tokens, &i))
					return (false);
			}
			else if (tokens[i] == "index")
			{
				std::string key = tokens[i];
				++i;
				if (tokens[i] == ";")
				{
					--i;
					std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
					return (false);
				}
				std::vector <std::string> values;
				while (tokens[i] != ";" && i < tokens.size())
				{
					values.push_back(tokens[i]);
					++i;
				}
				if (i >= tokens.size())
				{
					--i;
					std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
					return (false);
				}
				if (!assign_vector_keyval(&server, key, values))
					return (false);
				++i;
			}
			else
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
				if (!assign_single_keyval(&server, key, value))
					return (false);
				++i;
			}
		}
		if (tokens[i] != "}")
		{
			--i;
			std::cout << "Config file error: expecting '}' after " << tokens[i] << std::endl;
			return (false);
		}
		++i;
		if (!check_server(&server))
			return (false);
		service->servers.push_back(server);
	}
	return (true);
}

bool Parser::parse_location(Server* server, std::vector <std::string> tokens, size_t* i)
{
	bool case_sensitive = true;

	if (tokens[*i] == "~")
		++(*i);
	else if (tokens[*i] == "~*")
	{
		case_sensitive = false;
		++(*i);
	}
	if (*i >= tokens.size())
	{
		--(*i);
		std::cout << "Config file error: unexpected element after " << tokens[*i] << std::endl;
		return (false);
	}
	std::string cgi_key;
	if (case_sensitive)
		cgi_key = tokens[*i];
	else
	{
		cgi_key = tokens[*i];
		for (size_t j = 0; j < cgi_key.size(); j++)
			cgi_key[j] = static_cast<char>(std::tolower(static_cast<unsigned char>(cgi_key[j])));
	}
	++(*i);
	if (*i >= tokens.size() || tokens[*i] != "{")
	{
		--(*i);
		std::cout << "Config file error: expecting '{' after " << tokens[*i] << std::endl;
		return (false);
	}
	++(*i);
	std::map<std::string, std::string> cgi_map;
	while (*i < tokens.size() && tokens[*i] != "}")
	{
		if (tokens[*i] == ";" || tokens[*i] == "{")
		{
			--(*i);
			std::cout << "Config file error: unexpected element after " << tokens[*i] << std::endl;
			return (false);
		}
		std::string key = tokens[*i];
		++(*i);
		if (*i >= tokens.size() || tokens[*i] == ";" || tokens[*i] == "{")
		{
			--(*i);
			std::cout << "Config file error: unexpected element after " << tokens[*i] << std::endl;
			return (false);
		}
		std::string value = tokens[*i];
		++(*i);
		if (*i >= tokens.size() || tokens[*i] != ";")
		{
			--(*i);
			std::cout << "Config file error: expecting ';' after " << tokens[*i] << std::endl;
			return (false);
		}
		cgi_map.insert(std::make_pair(key, value));
			++(*i);
	}
	if (*i >= tokens.size() || tokens[*i] != "}")
	{
		--(*i);
		std::cout << "Config file error: expecting '}' after " << tokens[*i] << std::endl;
		return (false);
	}
	if (cgi_map.size() == 0)
	{
		std::cout << "Config file error: CGI mapping empty " << std::endl;
		return (false);
	}
	server->insert_CGI(cgi_key, cgi_map);
	return (true);
}

bool Parser::assign_single_keyval(Server* server, std::string& key, std::string& value)
{
	std::string fields[6] = {"server_name", "listen", "host", "root", "error_page", "client_max_body_size"};
	int field = -1;

	for (int i = 0; i < 6; i++)
	{
		if (key == fields[i])
			field = i;
	}
	switch (field)
	{
		case 0:
			if (!server->get_name().empty())
			{
				std::cout << "Config file error: duplicate server name" << std::endl;
				return (false);
			}
			server->set_name(value);
			break ;
		case 1:
			if (server->get_port() != -1)
			{
				std::cout << "Config file error: duplicate port" << std::endl;
				return (false);
			}
			server->set_port(atoi(value.c_str()));
			break ;
		case 2:
			if (!server->get_host().empty())
			{
				std::cout << "Config file error: duplicate host" << std::endl;
				return (false);
			}
			server->set_host(value);
			break ;
		case 3:
			if (!server->get_root().empty())
			{
				std::cout << "Config file error: duplicate root" << std::endl;
				return (false);
			}
			server->set_root(value);
			break ;
		case 4:
			if (!server->get_error_page().empty())
			{
				std::cout << "Config file error: duplicate error page" << std::endl;
				return (false);
			}
			server->set_error_page(value);
			break ;
		case 5:
			if (server->get_client_max_body_size() != -1)
			{
				std::cout << "Config file error: duplicate max body size" << std::endl;
				return (false);
			}
			server->set_client_max_body_size(atoi(value.c_str()));
			break;
		default:
				std::cout << "Config file error: field not valid" << std::endl;
				return (false);
	}
	return (true);
}

bool Parser::assign_vector_keyval(Server* server, std::string& key, std::vector <std::string> values)
{
	std::string fields[1] = {"index"};
	int field = -1;

	for (int i = 0; i < 1; i++)
	{
		if (key == fields[i])
			field = i;
	}
	switch (field)
	{
		case 0:
			if (!server->get_index().empty())
			{
				std::cout << "Config file error: duplicate index" << std::endl;
				return (false);
			}
			server->set_index(values);
			break ;
		default:
			return (false);
	}
	return (true);
}

bool Parser::check_server(Server* server)
{
	if (server->get_name().empty())
	{
		std::cout << "Config file error: server name missing" << std::endl;
		return (false);
	}
	if (server->get_port() == -1)
	{
		std::cout << "Config file error: port missing" << std::endl;
		return (false);
	}
	if (server->get_host().empty())
	{
		std::cout << "Config file error: host missing" << std::endl;
		return (false);
	}
	if (server->get_root().empty())
	{
		std::cout << "Config file error: root missing" << std::endl;
		return (false);
	}
	if (server->get_index().empty())
	{
		std::cout << "Config file error: index missing" << std::endl;
		return (false);
	}
	if (server->get_error_page().empty())
	{
		std::cout << "Config file error: error page missing" << std::endl;
		return (false);
	}
	if (server->get_client_max_body_size() == -1)
	{
		std::cout << "Config file error: max client body size missing" << std::endl;
		return (false);
	}
	return (true);
}