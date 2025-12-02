#include "request.hpp"
#include "../Core/debugPrinting.hpp"

Request::Request() : _version(""),
					 _request_data(""),
					 _header(""),
					 _method(""),
					 _uri(""),
					 _host(""),
					 _header_kv(),
					 _content_length(0),
					 _fullPathURI(""),
					 _root(""),
					 _isDirectory(false),
					 _stat(),
					 _isCGI(false),
					 _client_max_body_size(0),
					 _index(),
					 _cgi_path(""),
					 _autoindex(false),
					 _body(""),
					 _body_kv(),
					 _multiparts(),
					 _body_data()
{
}

Request::Request(const Request &other) : _version(other._version),
										 _request_data(other._request_data),
										 _header(other._header),
										 _method(other._method),
										 _uri(other._uri),
										 _host(other._host),
										 _header_kv(other._header_kv),
										 _content_length(other._content_length),
										 _fullPathURI(other._fullPathURI),
										 _root(other._root),
										 _isDirectory(other._isDirectory),
										 _stat(other._stat),
										 _isCGI(other._isCGI),
										 _client_max_body_size(other._client_max_body_size),
										 _index(other._index),
										 _cgi_path(other._cgi_path),
										 _autoindex(other._autoindex),
										 _body(other._body),
										 _body_kv(other._body_kv),
										 _multiparts(other._multiparts),
										 _body_data(other._body_data)
{
}

Request &Request::operator=(const Request &other)
{
	if (this != &other)
	{
		_version = other._version;
		_request_data = other._request_data;
		_header = other._header;
		_method = other._method;
		_uri = other._uri;
		_host = other._host;
		_header_kv = other._header_kv;
		_content_length = other._content_length;
		_fullPathURI = other._fullPathURI;
		_root = other._root;
		_isDirectory = other._isDirectory;
		_stat = other._stat;
		_isCGI = other._isCGI;
		_client_max_body_size = other._client_max_body_size;
		_index = other._index;
		_cgi_path = other._cgi_path;
		_autoindex = other._autoindex;
		_body = other._body;
		_body_kv = other._body_kv;
		_multiparts = other._multiparts;
		_body_data = other._body_data;
	}
	return (*this);
}

Request::~Request() {}

/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/

int Request::parse_header()
{
	print_blue("----- Parsing header... -----", DEBUG);

	while (!_header.empty() && (_header[0] == '\r' || _header[0] == '\n'))
		_header.erase(0, 1);
	std::string line;
	std::istringstream stream(_header);
	std::getline(stream, line);
	size_t first_space = line.find(" ");
	size_t second_space = line.find(" ", first_space + 1);

	if (first_space != std::string::npos && second_space != std::string::npos)
	{
		_method = line.substr(0, first_space);
		_uri = line.substr(first_space + 1, second_space - first_space - 1);
		if (_uri.size() > 2048)
			return (1);
		_version = line.substr(second_space + 1);
		if (!_version.empty() && _version.substr(_version.size() - 1) == "\r")
			_version.erase(_version.size() - 1);
	}
	while (std::getline(stream, line))
	{
		size_t colon_pos = line.find(":");
		if (colon_pos != std::string::npos)
		{
			std::string key = line.substr(0, colon_pos);
			if (key.empty())
			{
				print_red("Error: empty key", DEBUG);
				return (1);
			}
			key.erase(key.find_last_not_of(" ") + 1);
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			std::string value = line.substr(colon_pos + 1);
			value.erase(0, value.find_first_not_of(" "));

			if (!value.empty() && value.substr(value.size() - 1) == "\r")
				value.erase(value.size() - 1);

			if ((key.empty() && !value.empty()) || (!key.empty() && value.empty()))
			{
				print_red("Error: malformed header", DEBUG);
				return (1);
			}
			if (!_header_kv.count(key))
				_header_kv[key] = value;
			else
			{
				print_red("Error: duplicate keys in header", DEBUG);
				return (1);
			}
		}
		else
			continue;
	}

	_host = _header_kv["host"];
	_header_kv.erase("host");
	if (!_header_kv.count("content-length"))
		_content_length = 0;
	else
	{
		char *endptr;
		long val = std::strtol(_header_kv["content-length"].c_str(), &endptr, 10);
		if (*endptr != '\0' && *endptr != '\r')
		{
			print_red("Error: Bad formatting: content-length", DEBUG);
			return (1);
		}
		if (val < 0)
		{
			print_red("Error: Bad formatting: content-length negative", DEBUG);
			return (1);
		}
		_content_length = val;
	}

	print_cyan("> 'method: '" + _method + "'", DEBUG);
	print_cyan("> 'uri: '" + _uri + "'", DEBUG);
	print_cyan("> 'version: '" + _version + "'", DEBUG);
	print_cyan("> 'host: '" + _host + "'", DEBUG);
	std::map<std::string, std::string>::iterator it;
	for (it = _header_kv.begin(); it != _header_kv.end(); ++it)
		print_cyan("> '" + it->first + "': '" + it->second + "'", DEBUG);
	print_blue("----- Header parsed sucessfully! -----\n", DEBUG);
	return (0);
}

int Request::parse_body()
{
	print_blue("----- Parsing body... -----", DEBUG);
	std::string content_type = _header_kv["content-type"];
	if (content_type == "application/x-www-form-urlencoded")
		return (parse_url_encoded());
	else if (content_type.find("multipart/form-data") != std::string::npos)
		return (parse_multipart(content_type));
	else
		return (treat_as_raw_body());
}
