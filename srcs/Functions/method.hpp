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

		/* GET method */
		static void	handle_get(Client& client);
		static void	get_file(Client &client, std::string filepath);
		static void	determine_content_type(Client &client, std::string filepath);
		static void	get_directory(Client &client, DIR* directory);

		/* POST method */
		static void	handle_post(Client& client);
		static int	input_data(Client& client);
		static std::string	double_quote_handling(const std::string& string);
		static int	save_uploaded_files(Client& client, std::vector<MultiPart>& parts, const std::string& upload_directory);

		/* DELETE method */
		static void	handle_delete(Client& client);
};

#endif
