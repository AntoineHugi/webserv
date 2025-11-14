#ifndef RESPONSE_H
# define RESPONSE_H

# include <string>
# include <vector>
# include <map>
# include <iostream>

class Response
{
	private:

	public:
		std::string _response_data;
		std::string _header;
		std::string _body;
		size_t _content_length;
		std::vector <std::string>	_allowedMethods;

		Response();
		Response(const Response& other);
		Response& operator=(const Response& other);
		~Response();
};

#endif
