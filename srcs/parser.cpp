#include "parser.hpp"
#include "service.hpp"
#include <iostream>
#include <fstream>
#include <string>

Parser::Parser() {}

Parser::Parser(const Parser& other) 
{ 
	(void)other; 
}

Parser& Parser::operator=(const Parser& other) 
{
	(void)other;
	return (*this);
}

Parser::~Parser() {}

int	Parser::open_config(char *arg, Service* service)
{
	std::string filename = arg;
	if (filename.empty())
	{
		std::cerr << "Error: no file name" << std::endl;
		return (0);
	}
	std::ifstream file(filename.c_str());
	if (file.is_open())
	{
		if (!Parser::parse_file(file, service))
		{
			file.close();
			return (0);
		}
		else
			return (1);
	}
	else
	{
		std::cerr << "Error: couldn't open file" << std::endl;
		return (0);
	}
}

int	Parser::parse_file(std::ifstream& file, Service* service)
{
	std::string	line;
	bool	curly_bracket = false;
	int	server_count = 0;

	
}