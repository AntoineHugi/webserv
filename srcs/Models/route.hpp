#ifndef ROUTE_H
#define ROUTE_H

#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <algorithm>

class Route
{
	private:
		std::string _path;
		std::string _root;
		std::string _autoindex;
		std::string _cgi_path;
		unsigned long _client_max_body_size;
		std::vector<std::string> _methods;
		std::vector<std::string> _index;
		std::map<std::string, std::string> _bouncer;
		bool _cgi;
		std::map <int, std::string> _redirect;

	public:
		Route();
		Route(const Route &other);
		Route &operator=(const Route &other);
		~Route();

		std::string get_path() const { return (_path); }
		std::string get_root() const { return (_root); }
		std::string get_autoindex() const { return (_autoindex); }
		std::string get_cgi_path() const { return (_cgi_path); }
		unsigned long get_client_max_body_size() const { return (_client_max_body_size); }
		std::vector<std::string> get_methods() const { return (_methods); }
		std::vector<std::string> get_index() const { return (_index); }
		std::map<std::string, std::string> get_bouncer() const { return (_bouncer); }
		bool get_cgi() const { return (_cgi); }
		std::map<int, std::string> get_redirect() const { return (_redirect); }

		void set_path(const std::string &path) { _path = path; }
		void set_root(const std::string &root) { _root = root; }
		void set_cgi_path(const std::string &cgi_path) { _cgi_path = cgi_path; }
		void add_bouncer(std::string &key, std::string &value) { _bouncer.insert(std::make_pair(key, value)); }
		void set_index(std::vector<std::string> index) { _index = index; }
		void set_cgi(bool cgi) { _cgi = cgi; }
		void set_redirect(int key, std::string &value) { _redirect.insert(std::make_pair(key, value)); }

		void set_autoindex(const std::string &autoindex);
		void set_client_max_body_size(const std::string &max);
		void set_methods(const std::vector<std::string> &methods);

};

#endif
