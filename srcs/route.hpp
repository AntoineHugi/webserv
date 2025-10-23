#ifndef ROUTE_H
# define ROUTE_H

# include <vector>
# include <string>
# include <stdexcept>
# include <unistd.h>

class Route
{
	private:
		std::string	_path;
		std::string	_root;
		std::vector <std::string>	_methods;
		std::string	_autoindex;
		std::string	_cgi_path;

	public:
		Route();
		Route(const Route& other);
		Route& operator=(const Route& other);
		~Route();

		std::string get_path() const;
		std::string get_root() const;
		std::vector <std::string> get_methods() const;
		std::string	get_autoindex() const;
		std::string	get_cgi_path() const;

		void	set_path(const std::string& path);
		void	set_root(const std::string& root);
		void	set_methods(const std::vector <std::string>& methods);
		void	set_autoindex(const std::string& autoindex);
		void	set_cgi_path(const std::string& cgi_path);
};

#endif