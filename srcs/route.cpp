# include "route.hpp"

Route::Route(): _path(""), _root(""), _methode(), _autoindex("") {}

Route::Route(const Route& other)
{
	this->_path = other._path;
	this->_root = other._root;
	this->_methode = other._methode;
	this->_autoindex = other._autoindex;
}

Route& Route::operator=(const Route& other)
{
	if (this != &other)
	{
		this->_path = other._path;
		this->_root = other._root;
		this->_methode = other._methode;
		this->_autoindex = other._autoindex;
	}
	return (*this);
}

Route::~Route() {}


void	Route::set_path(std::string path) { this->_path = path; }
void	Route::set_root(std::string root) { this->_root = root; }
void	Route::set_methode(std::vector <std::string> methode) { this->_methode = methode; }
void	Route::set_autoindex(std::string autoindex) { this->_autoindex = autoindex; }
void	Route::set_deny(std::string deny) { this->_deny = deny; }
void	Route::set_cgi_path(std::string cgi_path) { this->_cgi_path = cgi_path; }

std::string	Route::get_path() { return (this->_path); }
std::string	Route::get_root() { return (this->_root); }
std::vector <std::string>	Route::get_methode() { return (this->_methode); }
std::string	Route::get_autoindex() { return (this->_autoindex); }
std::string	Route::get_deny() { return (this->_deny); }
std::string	Route::get_cgi_path() { return (this->_cgi_path); }