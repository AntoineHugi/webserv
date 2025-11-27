#include "route.hpp"

Route::Route() : _path(""), _root(""), _autoindex(""),
				 _cgi_path(""), _client_max_body_size(0), _methods(), 
				 _index(), _bouncer(), _cgi(false), 
				 _redirect() {}

Route::Route(const Route &other)
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

std::string Route::get_path() const { return (_path); }
std::string Route::get_root() const { return (_root); }
std::string Route::get_autoindex() const { return (_autoindex); }
std::string Route::get_cgi_path() const { return (_cgi_path); }
unsigned long Route::get_client_max_body_size() const { return (_client_max_body_size); }
std::vector<std::string> Route::get_methods() const { return (_methods); }
std::vector<std::string> Route::get_index() const { return (_index); }
std::map<std::string, std::string> Route::get_bouncer() const { return (_bouncer); }
bool Route::get_cgi() const { return (_cgi); }

void Route::set_path(const std::string &path) { _path = path; }
void Route::set_root(const std::string &root) { _root = root; }
void Route::set_autoindex(const std::string &autoindex)
{
	if (autoindex != "on" && autoindex != "off")
		throw(std::runtime_error("Invalid autoindex: " + autoindex));
	_autoindex = autoindex;
}
std::map<int, std::string> Route::get_redirect() const { return (_redirect); }

void Route::set_cgi_path(const std::string &cgi_path) { _cgi_path = cgi_path; }
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

void Route::set_index(std::vector<std::string> index) { _index = index; }
void Route::add_bouncer(std::string &key, std::string &value) { _bouncer.insert(std::make_pair(key, value)); }
void Route::set_cgi(bool cgi) { _cgi = cgi; }
void Route::set_redirect(int key, std::string& value) { _redirect.insert(std::make_pair(key, value)); }
