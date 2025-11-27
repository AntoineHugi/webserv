#ifndef METHOD_H
# define METHOD_H

# include <string>
# include <vector>
# include <map>
# include <iostream>
# include <fstream>
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

		static void	handle_get(Client& client);
		static void	get_file(Client &client, std::string filepath);
		static void	determine_content_type(Client &client, std::string filepath);
		static void	get_directory(Client &client, DIR* directory);
		static void	handle_post(Client& client);
		static int	saveUploadedFiles(std::vector<MultiPart>& parts, const std::string& upload_directory);
		static void	handle_delete(Client& client);
};

#endif
