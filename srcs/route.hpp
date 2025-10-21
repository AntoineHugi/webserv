#ifndef ROUTE_H
# define ROUTE_H

# include <vector>
# include <string>

class Route
{
	private:
		std::string	_path;
		std::string	_root;
		std::vector <std::string>	_methode;
		std::string	_autoindex;
		std::string	_deny;

	public:
		Route();
		Route(const Route& other);
		Route& operator=(const Route& other);
		~Route();

		void	set_path(std::string path);
		void	set_root(std::string root);
		void	set_methode(std::vector <std::string> methode);
		void	set_autoindex(std::string autoindex);
		
		std::string get_path();
		std::string get_root();
		std::vector <std::string> get_methode();
		std::string	get_autoindex();
};

#endif