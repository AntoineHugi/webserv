# include "route.hpp"

Route::Route(): _path(""), _root(""), _autoindex(""), _cgi_path(""), _redirect(""), _client_max_body_size(0), _methods(), _index(), _bouncer() {}

Route::Route(const Route& other)
{
	_path = other._path;
	_root = other._root;
	_autoindex = other._autoindex;
	_cgi_path = other._cgi_path;
	_redirect = other._redirect;
	_client_max_body_size = other._client_max_body_size;
	_methods = other._methods;
	_index = other._index;
	_bouncer = other._bouncer;
}

Route& Route::operator=(const Route& other)
{
	if (this != &other)
	{
		_path = other._path;
		_root = other._root;
		_autoindex = other._autoindex;
		_cgi_path = other._cgi_path;
		_redirect = other._redirect;
		_client_max_body_size = other._client_max_body_size;
		_methods = other._methods;
		_index = other._index;
		_bouncer = other._bouncer;
	}
	return (*this);
}

Route::~Route() {}

std::string	Route::get_path() const { return (_path); }
std::string	Route::get_root() const { return (_root); }
std::string	Route::get_autoindex() const { return (_autoindex); }
std::string	Route::get_cgi_path() const { return (_cgi_path); }
std::string	Route::get_redirect() const { return (_redirect); }
unsigned long		Route::get_client_max_body_size() const { return (_client_max_body_size); }
std::vector <std::string>	Route::get_methods() const { return (_methods); }
std::vector <std::string>	Route::get_index() const { return (_index); }
std::map <std::string, std::string>	Route::get_bouncer() const { return (_bouncer); }

void	Route::set_path(const std::string& path) { _path = path; }
void	Route::set_root(const std::string& root) { _root = root; }
void	Route::set_autoindex(const std::string& autoindex) 
{
	if (autoindex != "on" && autoindex != "off")
		throw (std::runtime_error("Invalid autoindex: " + autoindex));
	_autoindex = autoindex;
}

void	Route::set_cgi_path(const std::string& cgi_path) { _cgi_path = cgi_path; }
void	Route::set_redirect(const std::string& redirect) { _redirect = redirect; }
void	Route::set_client_max_body_size(const std::string& max)
{
	if (max.empty())
		throw (std::runtime_error("Invalid max client body size - empty string"));
	errno = 0;
	char* last;
	unsigned long value = std::strtoul(max.c_str(), &last, 10);
	if (*last != '\0')
	{
		if (last[1] != '\0')
			throw (std::runtime_error("Invalid max client body size - invalid format"));
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
			throw (std::runtime_error("Invalid max client body size - wrong unit suffix"));
	}
	this->_client_max_body_size = value;
}

void	Route::set_methods(const std::vector <std::string>& methods) 
{
	for (size_t i = 0; i < methods.size(); i++)
	{
		if (methods[i] != "GET" && methods[i] != "POST" && methods[i] != "DELETE")
			throw (std::runtime_error("Invalid method: " + methods[i]));
	}
	_methods = methods;
}

void	Route::set_index(std::vector <std::string> index) { _index = index; }
void	Route::add_bouncer(std::string& key, std::string& value) { _bouncer.insert(std::make_pair(key, value)); }

