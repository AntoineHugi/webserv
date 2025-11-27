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

	std::string get_path() const;
	std::string get_root() const;
	std::string get_autoindex() const;
	std::string get_cgi_path() const;
	unsigned long get_client_max_body_size() const;
	std::vector<std::string> get_methods() const;
	std::vector<std::string> get_index() const;
	std::map<std::string, std::string> get_bouncer() const;
	bool get_cgi() const;
	std::map <int, std::string> get_redirect() const;

	void set_path(const std::string &path);
	void set_root(const std::string &root);
	void set_autoindex(const std::string &autoindex);
	void set_cgi_path(const std::string &cgi_path);
	void set_client_max_body_size(const std::string &max);
	void set_methods(const std::vector<std::string> &methods);
	void add_bouncer(std::string &key, std::string &value);
	void set_index(std::vector<std::string> index);
	void set_cgi(bool cgi);
	void set_redirect(int key, std::string& value);
};

#endif
