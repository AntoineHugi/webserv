#include "request.hpp"

Request::Request() : _request_data(""),
					 _header(""),
					 _method(""),
					 _uri(""),
					 _version(""),
					 _header_kv(),
					 _content_length(0),
					 _fullPathURI(""),
					 _root(""),
					 _isDirectory(false),
					 _stat(),
					 _isCGI(false),
					 _body(""),
					 _body_kv(),
					 _multiparts(),
					 _body_data()
{
}

Request::Request(const Request &other)
{
	_request_data = other._request_data;
	_header = other._header;
	_method = other._method;
	_uri = other._uri;
	_version = other._version;
	_header_kv = other._header_kv;
	_content_length = other._content_length;
	_fullPathURI = other._fullPathURI;
	_root = other._root;
	_isDirectory = other._isDirectory;
	_stat = other._stat;
	_isCGI = other._isCGI;
	_body = other._body;
	_body_kv = other._body_kv;
	_multiparts = other._multiparts;
	_body_data = other._body_data;
}

Request &Request::operator=(const Request &other)
{
	if (this != &other)
	{
		_request_data = other._request_data;
		_header = other._header;
		_method = other._method;
		_uri = other._uri;
		_version = other._version;
		_header_kv = other._header_kv;
		_content_length = other._content_length;
		_fullPathURI = other._fullPathURI;
		_root = other._root;
		_isDirectory = other._isDirectory;
		_stat = other._stat;
		_isCGI = other._isCGI;
		_body = other._body;
		_body_kv = other._body_kv;
		_multiparts = other._multiparts;
		_body_data = other._body_data;
	}
	return (*this);
}

Request::~Request() {}


int Request::parse_header()
{
	std::cout << "\033[34mParsing header...\n\033[0m" << std::endl;
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
			key.erase(key.find_last_not_of(" ") + 1);
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			std::string value = line.substr(colon_pos + 1, line.size() - 1);
			value.erase(0, value.find_first_not_of(" "));

			if (!value.empty() && value.substr(value.size() - 1) == "\r")
				value.erase(value.size() - 1);

			if (!_header_kv.count(key))
				_header_kv[key] = value;
			else
			{
				std::cout << "duplicate keys in header" << std::endl;
				return (1);
			}
		}
	}
	if (!_header_kv.count("content-length"))
		_content_length = 0;
	else
	{
		char *endptr;
		long val = std::strtol(_header_kv["content-length"].c_str(), &endptr, 10);
		if (*endptr != '\0' && *endptr != '\r')
		{
			std::cout << "Bad formatting: content-length" << std::endl;
			return (1);
		}
		_content_length = val;
	}

	/* printing header */
	std::map<std::string, std::string>::iterator it;
	std::cout << "'method: '" << _method << "'" << std::endl;
	std::cout << "'uri: '" << _uri << "'" << std::endl;
	std::cout << "'version : '" << _version << "'" << std::endl;
	for (it = _header_kv.begin(); it != _header_kv.end(); ++it)
	{
		std::cout << "'" << it->first << "' : '" << it->second << "'" << std::endl;
	}

	std::cout << "\033[34mHeader parsed sucessfully! \n\033[0m" << std::endl;
	return (0);
}

int Request::parse_body()
{
	std::cout << "\033[36mParsing body...\n\033[0m" << std::endl;
	std::cout << "\033[36m    Body: " << _body << "\n\033[0m" << std::endl;

	std::string content_type = _header_kv["content-type"];
	if (content_type == "application/x-www-form-urlencoded")
		return (parse_url_encoded());
	else if (content_type.find("multipart/form-data") != std::string::npos)
		return (parse_multipart(content_type));
	else if (content_type == "application/json")
		return (parse_json());
	else
		return (treat_as_raw_body());
}
