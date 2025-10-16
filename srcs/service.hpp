#ifndef SERVICE_H
# define SERVICE_H

# include <string>
# include <vector>
# include "server.hpp"

class Service
{
	private:
	
	public:
		std::vector<Server> servers;

		Service();
		Service(const Service& other);
		Service& operator=(const Service& other);
		~Service();
};

#endif