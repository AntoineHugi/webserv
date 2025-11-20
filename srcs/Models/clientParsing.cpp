
#include "client.hpp"

bool Client::validate_permissions()
{
	/* looks the best matching route for that uri */
	std::vector<Route> routes = _server->get_routes();
	if (routes.empty())
	{
		Route default_route;
		std::vector<std::string> methods;
		methods.push_back("GET");
		default_route.set_path("/");
		default_route.set_methods(methods);
		routes.push_back(default_route);
	}
	int index = -1;
	std::string matchedPath;
	for (size_t i = 0; i < routes.size(); ++i)
	{
		std::string path = routes[i].get_path();
		if (_request._uri.compare(0, path.size(), path) == 0)
		{
			if (path.size() >= matchedPath.size())
			{
				matchedPath = path;
				index = i;
			}
		}
	}
	if (matchedPath.empty() || index == -1)
	{
		set_status_code(404);
		return (false);
	}

	/* sets the right root for security: default or specific to the route and creates the full path root + uri */
	if (!routes[index].get_root().empty())
	{
		_request._root = routes[index].get_root();
		_request._fullPathURI = routes[index].get_root() + _request._uri.substr(matchedPath.size());
	}
	else
	{
		_request._root = _server->get_root();
		_request._fullPathURI = _server->get_root() + _request._uri.substr(matchedPath.size());
	}

	/* checks that the target uri exists */
	if (stat(_request._fullPathURI.c_str(), &_request._stat) != 0)
	{
		std::cout << "URI doesn't exist : " << _request._fullPathURI << std::endl;
		set_status_code(404);
		return (false);
	}
	std::cout << "URI exists : " << _request._fullPathURI << std::endl;

	/* checks if it's a directory */
	if (S_ISDIR(_request._stat.st_mode) && (_request._method == "GET" || _request._method == "POST"))
	{
		if ((!routes[index].get_autoindex().empty() && _request._method == "GET") || _request._method == "POST")
			_request._isDirectory = true;
		else
		{
			set_status_code(403);
			return (false);
		}
	}

	/* checks that the method is allowed by that route, and then validates that method's permissions */
	for (size_t j = 0; j < routes[index].get_methods().size(); ++j)
	{
		if (_request._method == routes[index].get_methods()[j])
		{
			if (validate_methods())
				return (true);
			else
				return (false);
		}
	}
	set_status_code(405);
	_response.set_allowed_methods(routes[index].get_methods());
	return (false);
}

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
	else if (_request._method == "POST")
	{
		/* checking if we can post in this directory */
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
	else if (_request._method == "DELETE" || _request._method == "POST")
	{
		/* checking that the path to the file for deletion is located within the root of this route / server class */
		char resolvedPath[PATH_MAX];
		char resolvedRoot[PATH_MAX];
		if (!realpath(_request._fullPathURI.c_str(), resolvedPath) || !realpath(_request._root.c_str(), resolvedRoot))
		{
			set_status_code(403);
			return (false);
		}
		std::string resolvedFile(resolvedPath);
		std::string resolvedRootDir(resolvedRoot);
		if (resolvedRootDir[resolvedRootDir.size() - 1] != '/')
			resolvedRootDir += '/';
		if (resolvedFile.find(resolvedRootDir) != 0)
		{
			set_status_code(403);
			return (false);
		}

		/* checking if we can delete in this directory */
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

int Client::read_to_buffer()
{
	char buf[50];
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
	std::cout << "try to parse body" << std::endl;
	/* send to chunked version */
	if (is_body_chunked())
	{
		std::cout << "body is chunked" << std::endl;
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
	/* Checking if we are still chekcking the header, if we're exceeding the size and if the end of header indicator has been found */
	if (_request._request_data.size() > 16384)
	{
		_status_code = 413;
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
	if (static_cast<long long>(_request._content_length) > _server->get_client_max_body_size())
	{
		_status_code = 413;
		set_create_response();
		return (1);
	}
	if (_request.http_requirements_met() != 200)
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
