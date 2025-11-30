#include "route.hpp"

Route::Route() :
	_path(""),
	_root(""),
	_autoindex(""),
	_cgi_path(""),
	_client_max_body_size(0),
	_methods(),
	_index(),
	_bouncer(),
	_cgi(false),
	_redirect()
{}

Route::Route(const Route &other) :
	_path(other._path),
	_root(other._root),
	_autoindex(other._autoindex),
	_cgi_path(other._cgi_path),
	_client_max_body_size(other._client_max_body_size),
	_methods(other._methods),
	_index(other._index),
	_bouncer(other._bouncer),
	_cgi(other._cgi),
	_redirect(other._redirect)
{}

Route &Route::operator=(const Route &other)
{
	if (this != &other)
	{
		_path = other._path;
		_root = other._root;
		_autoindex = other._autoindex;
		_cgi_path = other._cgi_path;
		_client_max_body_size = other._client_max_body_size;
		_methods = other._methods;
		_index = other._index;
		_bouncer = other._bouncer;
		_cgi = other._cgi;
		_redirect = other._redirect;
	}
	return (*this);
}

Route::~Route() {}

/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/
/*####################################################################################################*/

void Route::set_autoindex(const std::string &autoindex)
{
	if (autoindex != "on" && autoindex != "off")
		throw(std::runtime_error("Invalid autoindex: " + autoindex));
	_autoindex = autoindex;
}

void Route::set_client_max_body_size(const std::string &max)
{
	if (max.empty())
		throw(std::runtime_error("Invalid max client body size - empty string"));
	errno = 0;
	char *last;
	unsigned long value = std::strtoul(max.c_str(), &last, 10);
	if (*last != '\0')
	{
		if (last[1] != '\0')
			throw(std::runtime_error("Invalid max client body size - invalid format"));
		if (last == max.c_str())
			throw std::runtime_error("Invalid max client body size - no digits");
		char suffix = std::tolower(*last);
		if (suffix == 'k')
			value *= 1024;
		else if (suffix == 'm')
			value *= 1024 * 1024;
		else if (suffix == 'g')
			value *= 1024 * 1024 * 1024;
		else
			throw(std::runtime_error("Invalid max client body size - wrong unit suffix"));
	}
	this->_client_max_body_size = value;
}

void Route::set_methods(const std::vector<std::string> &methods)
{
	for (size_t i = 0; i < methods.size(); i++)
	{
		if (methods[i] != "GET" && methods[i] != "POST" && methods[i] != "DELETE")
			throw(std::runtime_error("Invalid method: " + methods[i]));
	}
	_methods = methods;
}
