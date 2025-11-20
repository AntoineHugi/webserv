#ifndef METHOD_H
# define METHOD_H

# include <string>
# include <vector>
# include <map>
# include <iostream>
# include <dirent.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdlib.h>

# include "../Models/client.hpp"

class Method
{
	private:

	public:

		Method();
		Method(const Method& other);
		Method& operator=(const Method& other);
		~Method();

		static void	handleGet(Client& client);
		static void	getFile(Client &client, std::string filepath);
		static void	getDirectory(Client &client, DIR* directory);
		static void	handlePost(Client& client);
		static void	handleDelete(Client& client);
};

#endif
