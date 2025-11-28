
#include "client.hpp"

bool Client::validate_methods()
{
	if (_request._method == "GET")
	{
		/* checking if we can read/execute this file/directory */
		if (_request._isCGI && access(_request._fullPathURI.c_str(), X_OK) != 0)
		{
			set_status_code(403);
			return (false);
		}
		else if (!_request._isCGI && access(_request._fullPathURI.c_str(), R_OK) != 0)
		{
			set_status_code(403);
			return (false);
		}
	}
	else if (_request._method == "POST" || _request._method == "DELETE")
	{
		/* checking if we can write in this directory */
		std::string dirPath;
		{
			char tmp[PATH_MAX];
			strncpy(tmp, _request._fullPathURI.c_str(), PATH_MAX);
			tmp[PATH_MAX - 1] = '\0';

			char *d = dirname(tmp);
			dirPath = std::string(d);
		}
		if (access(dirPath.c_str(), W_OK) != 0)
		{
			set_status_code(403);
			return (false);
		}
	}
	return (true);
}

bool Client::is_method_allowed(const Route &route)
{
	for (size_t j = 0; j < route.get_methods().size(); ++j)
	{
		if (_request._method == route.get_methods()[j])
			return (true);
	}
	set_status_code(405);
	_response.set_allowed_methods(route.get_methods());
	return (false);
}

bool Client::check_directory_rules(const Route &route)
{
	if (S_ISDIR(_request._stat.st_mode) && (_request._method == "GET" || _request._method == "POST"))
	{
		if ((!route.get_autoindex().empty() && _request._method == "GET") || _request._method == "POST")
			_request._isDirectory = true;
		else
		{
			set_status_code(403);
			return (false);
		}
	}
	return (true);
}

bool Client::check_subnet(const std::string &rule_target, const std::string &client_ip)
{
	size_t slash = rule_target.find('/');
	if (slash != std::string::npos && rule_target.size() >= slash + 2)
	{
		std::string network = rule_target.substr(0, slash);
		int prefix = std::strtol(rule_target.substr(slash + 1).c_str(), NULL, 10);
		if (prefix < 0 || prefix > 32)
		{
			set_status_code(500);
			set_create_response();
			return false;
		}

		struct in_addr addr;
		if (inet_pton(AF_INET, client_ip.c_str(), &addr) != 1)
		{
			set_status_code(500);
			set_create_response();
			return (false);
		}
		uint32_t client = ntohl(addr.s_addr);

		if (inet_pton(AF_INET, network.c_str(), &addr) != 1)
		{
			set_status_code(500);
			set_create_response();
			return (false);
		}
		uint32_t net = ntohl(addr.s_addr);

		uint32_t mask = 0;
		if (prefix > 0 && prefix <= 32)
		{
			mask = 0xFFFFFFFF;
			mask <<= (32 - prefix);
		}

		if ((client & mask) == (net & mask))
			return (true);
	}
	return (false);
}

bool Client::transversal_protection()
{
	/* checking that the absolute path to the root exists */
	char resolved_root[PATH_MAX];
	if (!realpath(_request._root.c_str(), resolved_root))
	{
		std::cout << "failed root : " << resolved_root << std::endl;
		return (false);
	}
	std::string real_root = resolved_root;
	if (real_root[real_root.size() - 1] != '/')
		real_root += '/';

	/* checking that the path to the file is located within the root */
	if (_request._method == "GET" || _request._method == "DELETE")
	{
		char resolved_file[PATH_MAX];
		if (!realpath(_request._fullPathURI.c_str(), resolved_file))
		{
			std::cout << "failed file path : " << resolved_file << std::endl;
			return (false);
		}
		std::string real_file(resolved_file);
		if (real_file[real_file.size() - 1] != '/')
			real_file += '/';
		if (real_file.find(real_root) != 0)
		{
			std::cout << "didn't find root " << real_root << " | the file is " << real_file << std::endl;
			return (false);
		}
	}
	/* checking that the path to the file parent directory is located within the root */
	else if (_request._method == "POST")
	{
		std::string parent = _request._fullPathURI;
		size_t lastSlash = parent.find_last_of('/');
		std::string lastComponent = (lastSlash == std::string::npos) ? parent : parent.substr(lastSlash + 1);
		if (lastComponent.find('.') != std::string::npos)
		{
			char *cpath = strdup(parent.c_str());
			parent = dirname(cpath);
			free(cpath);
		}

		char resolved_parent[PATH_MAX];
		if (!realpath(parent.c_str(), resolved_parent))
		{
			std::cout << "failed file path : " << resolved_parent << std::endl;
			return (false);
		}
		std::string real_parent = resolved_parent;
		if (real_parent[real_parent.size() - 1] != '/')
			real_parent += '/';
		if (real_parent.find(real_root) != 0)
		{
			std::cout << "didn't find root: " << real_root << " | in parent: " << real_parent << std::endl;
			return false;
		}
	}
	return (true);
}

bool Client::bouncer_approval(const Route &route)
{
	/* get highest ranking bouncer to decide on allow/deny, return if there is no bouncer */
	std::map<std::string, std::string> bouncer = route.get_bouncer();
	if (bouncer.empty())
		bouncer = _server->get_bouncer();
	if (bouncer.empty())
		return (true);

	for (std::map<std::string, std::string>::const_iterator it = bouncer.begin(); it != bouncer.end(); ++it)
	{
		const std::string &rule_type = it->first;
		const std::string &rule_target = it->second;

		if (rule_type == "deny")
		{
			if (rule_target == "all" || rule_target == _client_ip || check_subnet(rule_target, _client_ip))
				return false;
		}
		else if (rule_type == "allow")
		{
			if (rule_target == "none")
				return false;
			else if (rule_target == _client_ip)
				return (true);
			else if (check_subnet(rule_target, _client_ip))
				return (true);
		}
	}
	return (false);
}

bool Client::check_uri_exists()
{
	if (stat(_request._fullPathURI.c_str(), &_request._stat) != 0)
	{
		std::cout << "URI doesn't exist : " << _request._fullPathURI << std::endl;
		set_status_code(404);
		return (false);
	}
	return (true);
}

void Client::overwrite_with_route(const Route &route)
{
	if (route.get_client_max_body_size() > 0)
		_request._client_max_body_size = route.get_client_max_body_size();
	else
		_request._client_max_body_size = get_server()->get_client_max_body_size();

	if (!route.get_index().empty())
		_request._index = route.get_index();
	else
		_request._index = _server->get_index();

	if (!route.get_root().empty())
		_request._root = route.get_root();
	else
		_request._root = _server->get_root();

	if (!route.get_cgi_path().empty())
		_request._cgi_path = route.get_cgi_path();

	if (route.get_autoindex() == "on")
		_request._autoindex = true;
}

bool Client::route_matches(const std::string &uri, const std::string &route)
{
	if (uri.compare(0, route.size(), route) == 0)
	{
		if (uri.size() == route.size())
			return (true);
		else
		{
			if (uri[route.size()] == '/')
				return (true);
		}
	}
	return false;
}

std::vector<std::string> Client::fetch_extensions(const std::string &cgi_path)
{
	std::vector<std::string> extensions;

	if (cgi_path.size() < 3)
		return extensions;

	if (!(cgi_path[0] == '\\' && cgi_path[1] == '.'))
		return extensions;
	size_t pos = 2;

	bool multiple = false;
	if (pos < cgi_path.size() && cgi_path[pos] == '(')
	{
		multiple = true;
		++pos;
	}

	std::string current = ".";
	while (pos < cgi_path.size())
	{
		char c = cgi_path[pos];
		if (!std::isalnum(c) && c != '_' && c != '-' && c != '|' && c != ')' && c != '$')
			return (std::vector<std::string>());
		if (multiple == true)
		{
			if (c == ')')
			{
				++pos;
				if (pos >= cgi_path.size() || cgi_path[pos] != '$')
					return (std::vector<std::string>());
				if (current.size() > 1)
					extensions.push_back(current);
				return extensions;
			}
			if (c == '|')
			{
				if (current.size() > 1)
				{
					extensions.push_back(current);
					current = ".";
				}
			}
			/* this case is invalid inside a parenthesis */
			else if (c == '$')
				return std::vector<std::string>();
			else
				current += c;
		}
		else
		{
			if (c == ')' || c == '|')
				return std::vector<std::string>();
			if (c == '$')
			{
				if (current.size() > 1)
					extensions.push_back(current);
				return extensions;
			}
			if (c == '|' || c == ')')
				return std::vector<std::string>();
			current += c;
		}
		++pos;
	}
	return (std::vector<std::string>());
}

bool Client::cgi_matches(const std::string &uri, const std::string &cgi_path)
{
	std::vector<std::string> extensions = fetch_extensions(cgi_path);
	if (extensions.empty())
		return false;

	for (size_t i = 0; i < extensions.size(); ++i)
	{
		const std::string &extension = extensions[i];

		if (uri.size() >= extension.size() && uri.compare(uri.size() - extension.size(), extension.size(), extension) == 0)
			return true;
	}
	return false;
}

int Client::find_best_route_index(std::vector<Route> &routes)
{
	int best_index = -1;
	size_t best_len = 0;

	for (size_t i = 0; i < routes.size(); ++i)
	{
		const std::string &route_path = routes[i].get_path();
		if (routes[i].get_cgi())
		{
			if (cgi_matches(_request._uri, route_path))
				return (static_cast<int>(i));
		}
		else
		{
			if (route_matches(_request._uri, route_path))
			{
				if (route_path.size() >= best_len)
				{
					best_len = route_path.size();
					best_index = static_cast<int>(i);
				}
			}
		}
	}
	return (best_index);
}

bool Client::validate_permissions()
{
	std::vector<Route> routes = _server->get_routes();
	int routeIndex = find_best_route_index(routes);
	if (routeIndex < 0)
	{
		set_status_code(404);
		return (false);
	}

	const Route &route = routes[routeIndex];
	overwrite_with_route(route);
	if (!route.get_redirect().empty())
	{
		set_status_code(route.get_redirect().begin()->first);
		_response.set_location(route.get_redirect().begin()->second);
		return (false);
	}

	if (!route.get_cgi())
		_request._fullPathURI = _request._root + _request._uri.substr(route.get_path().size());
	else
	{
		size_t pos = _request._uri.find_last_of('/');
		std::string last;

		if (pos == std::string::npos)
			last = _request._uri;
		else if (pos + 1 < _request._uri.size())
			last = _request._uri.substr(pos + 1);
		else
			last = "";

		if (_request._root[_request._root.size() - 1] == '/')
			_request._root.erase(_request._root.size() - 1, _request._root.size());
		_request._fullPathURI = _request._root + '/' + last;
		_request._isCGI = true;
	}
	if (_request._method != "POST")
	{
		if (!check_uri_exists())
		{
			std::cout << "failed existing uri" << std::endl;
			return (false);
		}
	}
	if (!bouncer_approval(route))
	{
		std::cout << "failed bouncer" << std::endl;
		set_status_code(403);
		return (false);
	}
	if (!transversal_protection())
	{
		std::cout << "failed transversal" << std::endl;
		set_status_code(403);
		return (false);
	}
	if (!check_directory_rules(route))
	{
		std::cout << "failed directory rules" << std::endl;
		return (false);
	}
	if (!is_method_allowed(route))
	{
		std::cout << "failed allowed methods" << std::endl;
		return (false);
	}
	if (!validate_methods())
	{
		std::cout << "failed method validation" << std::endl;
		return (false);
	}

	return (true);
}

int Client::read_to_buffer()
{
	char buf[1048576];
	ssize_t n = recv(_fd, buf, sizeof(buf), 0);

	if (n > 0)
	{
		_request._request_data.append(buf, n);
		return 1;
	}
	else if (n == 0)
		return 0;
	else
		return -1;
}

bool Client::decode_chunked_body()
{
	std::string &raw = _request._request_data;
	size_t pos = 0;
	_request._body.clear();

	while (true)
	{
		size_t size_end = raw.find("\r\n", pos);
		if (size_end == std::string::npos)
			return (false);

		std::string hexsize = raw.substr(pos, size_end - pos);
		size_t chunk_size = strtoul(hexsize.c_str(), NULL, 16);
		pos = size_end + 2;

		if (chunk_size == 0)
			break;

		if (raw.size() < pos + chunk_size + 2)
			return (false);

		_request._body.append(raw, pos, chunk_size);
		pos += chunk_size + 2;
	}
	_request._request_data.erase(0, pos);
	if (!_request._request_data.empty())
		_flags._leftover_chunk = true;
	return (true);
}

bool Client::chunked_body_finished() const
{
	if (_request._request_data.find("0\r\n\r\n") != std::string::npos)
		return true;
	return false;
}

bool Client::try_parse_body()
{
	/* send to chunked version */
	if (_request._request_data.size() > _request._client_max_body_size)
	{
		_status_code = 413;
		set_create_response();
		return (1);
	}

	if (is_body_chunked())
	{
		//std::cout << "body is chunked" << std::endl;
		if (chunked_body_finished())
		{
			if (!decode_chunked_body())
			{
				_status_code = 400;
				set_create_response();
				return (1);
			}
			else
			{
				if (_request.parse_body())
				{
					_status_code = 400;
					set_create_response();
					return (1);
				}
				std::cout << "\033[35m  Body parsed \033[0m" << std::endl;
				set_process_request();
				return (0);
			}
		}
		return (0);
	}

	/* if we didn't get the get the whole data yet, skip for another turn of reading */
	if (_request._request_data.size() < _request._content_length)
		return (0);

	/* once we have everything, dump it into request._body */
	_request._body = _request._request_data.substr(0, _request._content_length);
	if (_request.parse_body())
	{
		_status_code = 400;
		set_create_response();
		return (1);
	}

// add check that content_length = body size if not chunked

	std::cout << "\033[35m  Body parsed \033[0m" << std::endl;
	_request._request_data.erase(0, _request._content_length);
	set_process_request();

	/* checking if there is anything left, potentially a new request */
	if (_request._request_data.size() > 0)
		_flags._leftover_chunk = true;
	else
		_flags._leftover_chunk = false;
	return (0);
}

bool Client::try_parse_header()
{
	/* Checking if we're exceeding the size and if the end of header indicator has been found */
	if (_request._request_data.size() > 16384)
	{
		_status_code = 431;
		set_create_response();
		return (1);
	}
	size_t pos = _request._request_data.find("\r\n\r\n");
	if (pos == std::string::npos)
		return (0);

	/* if found, parse the header and make some validation, jumping to the response if the parsing/validation fails */
	_request._header = _request._request_data.substr(0, pos + 4);
	if (_request.parse_header() != 0)
	{
		_status_code = 400;
		set_create_response();
		return (1);
	}
	_request._request_data.erase(0, pos + 4);
	set_flags();
	if (_request._request_data.size() > _request._content_length)
		_flags._leftover_chunk = true;
	if (static_cast<unsigned long>(_request._content_length) > _server->get_client_max_body_size())
	{
		_status_code = 413;
		set_create_response();
		return (1);
	}
	if ((_status_code = _request.http_requirements_met()) != 200)
	{
		set_create_response();
		std::cout << "failed requirements" << std::endl;
		return (1);
	}
	if (!validate_permissions())
	{
		set_create_response();
		if (_request._content_length > 0)
			_request._request_data.erase(0, std::min(_request._content_length, _request._request_data.size()));
		std::cout << "failed permissions" << std::endl;
		return (1);
	}
	std::cout << "\033[35m  Header parsed \033[0m" << std::endl;

	/* if validation works, check if we should read the body next or go to the processing step */
	if (is_body_chunked())
		set_read_body();
	else if (_request.http_can_have_body() && _request._content_length != 0)
		set_read_body();
	else
		set_process_request();
	return (0);
}
