# include "route.hpp"

Route::Route(): _path(""), _root(""), _autoindex(""), _cgi_path(""), _methods(), _bouncer() {}

Route::Route(const Route& other)
{
	_path = other._path;
	_root = other._root;
	_autoindex = other._autoindex;
	_cgi_path = other._cgi_path;
	_methods = other._methods;

}

Route& Route::operator=(const Route& other)
{
	if (this != &other)
	{
		_path = other._path;
		_root = other._root;
		_autoindex = other._autoindex;
		_cgi_path = other._cgi_path;
		_methods = other._methods;
	}
	return (*this);
}

Route::~Route() {}

std::string	Route::get_path() const { return (_path); }
std::string	Route::get_root() const { return (_root); }
std::string	Route::get_autoindex() const { return (_autoindex); }
std::string	Route::get_cgi_path() const { return (_cgi_path); }
std::vector <std::string>	Route::get_methods() const { return (_methods); }
std::map <std::string, std::string>	Route::get_bouncer() const { return (_bouncer); }

void	Route::set_path(const std::string& path) { _path = path; }
void	Route::set_root(const std::string& root) { _root = root; }
void	Route::set_autoindex(const std::string& autoindex) 
{
	if (autoindex != "on" && autoindex != "off")
			throw (std::runtime_error("Invalid autoindex: " + autoindex));
	_autoindex = autoindex;
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

void	Route::set_cgi_path(const std::string& cgi_path) { _cgi_path = cgi_path; }
void	Route::set_bouncer(std::string& key, std::string& value) { _bouncer.insert(std::make_pair(key, value)); }
