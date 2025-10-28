#ifndef CLIENT_H
# define CLIENT_H

# include <string>
# include <vector>

class Client
{
	private:

	public:
		std::string _request_data;

		Client();
		Client(const Client& other);
		Client& operator=(const Client& other);
		~Client();

};

#endif
