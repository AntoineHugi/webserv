#ifndef METHOD_H
# define METHOD_H

# include <string>
# include <vector>
# include <map>
# include <iostream>

# include "client.hpp"

class Method
{
	private:

	public:

		Method();
		Method(const Method& other);
		Method& operator=(const Method& other);
		~Method();

		static void	handleGet(Client& client);
		static void	handlePost(Client& client);
		static void	handleDelete(Client& client);
};

#endif
