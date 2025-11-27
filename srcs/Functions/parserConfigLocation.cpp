#include "parserConfig.hpp"

bool Parser::assign_single_keyval_route(Route& route, std::string &key, std::string &value)
{
	std::vector<std::string> fields;
	int field = -1;
	fields.push_back("root");
	fields.push_back("autoindex");
	fields.push_back("cgi_path");
	fields.push_back("client_max_body_size");

	for (size_t i = 0; i < fields.size(); i++)
	{
		if (key == fields[i])
			field = i;
	}
	switch (field)
	{
	case 0:
		if (route.get_root().empty() - 1)
		{
			std::cout << "Config file error: duplicate root line in location" << std::endl;
			return (false);
		}
		route.set_root(value);
		break;
	case 1:
		if (!route.get_autoindex().empty())
		{
			std::cout << "Config file error: duplicate autoindex line in location" << std::endl;
			return (false);
		}
		try
		{
			route.set_autoindex(value);
		}
		catch (const std::exception &e)
		{
			std::cout << "Config file error: " << e.what() << std::endl;
			return (false);
		}
		break;
	case 2:
		if (!route.get_cgi_path().empty())
		{
			std::cout << "Config file error: duplicate cgi path line in location" << std::endl;
			return (false);
		}
		route.set_cgi_path(value);
		break;
	case 3:
		if (route.get_client_max_body_size() > 0)
		{
			std::cout << "Config file error: duplicate cgi path line in location" << std::endl;
			return (false);
		}
		try
		{
			route.set_client_max_body_size(value);
		}
		catch (const std::exception &e)
		{
			std::cout << "Config file error: " << e.what() << std::endl;
			return (false);
		}
		break;
	default:
		std::cout << "Config file error: location field not valid" << std::endl;
		return (false);
	}
	return (true);
}

bool Parser::handle_location_keyval(Route &route, std::vector<std::string> &tokens, size_t &i)
{

	std::string key = tokens[i];
	++i;
	if (i >= tokens.size() || tokens[i] == ";" || tokens[i] == "{")
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
	if (!assign_single_keyval_route(route, key, value))
		return (false);
	return (true);
}

bool Parser::handle_location_allowance(Route &route, std::vector<std::string> &tokens, size_t &i)
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
	route.add_bouncer(key, value);
	return (true);
}

bool Parser::handle_location_index(Route &route, std::vector<std::string> &tokens, size_t &i)
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

	if (!route.get_index().empty())
	{
		std::cout << "Config file error: duplicate index line in location" << std::endl;
		return (false);
	}
	route.set_index(index);
	return (true);
}

bool Parser::handle_location_return(Route &route, std::vector<std::string> &tokens, size_t &i)
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
	route.set_redirect(key, value);
	++i;
	if (i >= tokens.size() || tokens[i] != ";")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	return (true);
}

bool Parser::handle_location_method(Route &route, std::vector<std::string> &tokens, size_t &i)
{
	++i;
	if (i >= tokens.size() || tokens[i] == ";" || tokens[i] == "{" || tokens[i] == "}")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	std::vector<std::string> method;
	method.push_back(tokens[i]);
	++i;
	while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "{" && tokens[i] != "}")
	{
		method.push_back(tokens[i]);
		++i;
	}
	if (i >= tokens.size() || tokens[i] != ";")
	{
		--i;
		std::cout << "Config file error: expecting ';' after " << tokens[i] << std::endl;
		return (false);
	}
	if (!route.get_methods().empty())
	{
		std::cout << "Config file error: duplicate methods line in location" << std::endl;
		return (false);
	}
	try
	{
		route.set_methods(method);
	}
	catch (const std::exception &e)
	{
		std::cout << "Config file error: " << e.what() << std::endl;
		return (false);
	}
	return (true);
}

bool Parser::check_duplicates(Server *server, std::string route)
{
	for (size_t j = 0; j < server->get_routes().size(); j++)
	{
		if (server->get_routes()[j].get_path() == (server->get_root() + route))
		{
			std::cout << "Config file error: duplicate route." << std::endl;
			return (false);
		}
	}
	return (true);
}

bool Parser::parse_location(Server *server, std::vector<std::string> &tokens, size_t &i)
{
	++i;
	if (i >= tokens.size() || tokens[i] == ";")
	{
		--i;
		std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
		return (false);
	}
	bool cgi = false;
	if (tokens[i] == "~")
	{
		cgi = true;
		++i;
	}

	if (!check_duplicates(server, tokens[i]))
		return (false);

	Route route;
	route.set_cgi(cgi);
	route.set_path(tokens[i]);
	++i;
	if (i >= tokens.size() || tokens[i] != "{")
	{
		--i;
		std::cout << "Config file error: expecting '{' after " << tokens[i] << std::endl;
		return (false);
	}
	++i;
	if (i >= tokens.size() || tokens[i] == "}")
	{
		std::cout << "Config file error: location block empty " << std::endl;
		return (false);
	}
	while (i < tokens.size() && tokens[i] != "}")
	{
		if (tokens[i] == ";" || tokens[i] == "{")
		{
			--i;
			std::cout << "Config file error: unexpected element after " << tokens[i] << std::endl;
			return (false);
		}
		if (tokens[i] == "methods")
		{
			if (!handle_location_method(route, tokens, i))
				return (false);
		}
		else if (tokens[i] == "return")
		{
			if (!handle_location_return(route, tokens, i))
				return (false);
		}
		else if (tokens[i] == "index")
		{
			if (!handle_location_index(route, tokens, i))
				return (false);
		}
		else if (tokens[i] == "allow" || tokens[i] == "deny")
		{
			if (!handle_location_allowance(route, tokens, i))
				return (false);
		}
		else
		{
			if (!handle_location_keyval(route, tokens, i))
				return (false);
		}
		++i;
	}
	if (i >= tokens.size() || tokens[i] != "}")
	{
		--i;
		std::cout << "Config file error: expecting '}' after " << tokens[i] << std::endl;
		return (false);
	}
	server->add_route(route);
	return (true);
}
