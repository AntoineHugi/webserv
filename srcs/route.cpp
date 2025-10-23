# include "route.hpp"

Route::Route(): _path(""), _root(""), _methods(), _autoindex(""), _cgi_path("") {}

Route::Route(const Route& other)
{
	this->_path = other._path;
	this->_root = other._root;
	this->_methods = other._methods;
	this->_autoindex = other._autoindex;
	this->_cgi_path = other._cgi_path;
}

Route& Route::operator=(const Route& other)
{
	if (this != &other)
	{
		this->_path = other._path;
		this->_root = other._root;
		this->_methods = other._methods;
		this->_autoindex = other._autoindex;
		this->_cgi_path = other._cgi_path;
	}
	return (*this);
}

Route::~Route() {}

std::string	Route::get_path() const { return (this->_path); }
std::string	Route::get_root() const { return (this->_root); }
std::vector <std::string>	Route::get_methods() const { return (this->_methods); }
std::string	Route::get_autoindex() const { return (this->_autoindex); }
std::string	Route::get_cgi_path() const { return (this->_cgi_path); }


void	Route::set_path(const std::string& path) { this->_path = path; }
void	Route::set_root(const std::string& root) { this->_root = root; }

void	Route::set_methods(const std::vector <std::string>& methods) 
{
	for (size_t i = 0; i < methods.size(); i++)
	{
		if (methods[i] != "GET" && methods[i] != "POST" && methods[i] != "DELETE")
			throw (std::runtime_error("Invalid method: " + methods[i]));
	}
	this->_methods = methods;
}

void	Route::set_autoindex(const std::string& autoindex) 
{
	if (autoindex != "on" && autoindex != "off")
			throw (std::runtime_error("Invalid autoindex: " + autoindex));
	this->_autoindex = autoindex;
}

void	Route::set_cgi_path(const std::string& cgi_path) { this->_cgi_path = cgi_path; }
