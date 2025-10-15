#ifndef SERVICE_H
# define SERVICE_H

# include <string>
# include "server.hpp"

class Service
{
	private:
		Server** servers;
		int**	fds;
	
	public:
		Service();
		Service(const Service& other);
		Service& operator=(const Service& other);
		~Service();
};

#endif