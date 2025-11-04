#ifndef SERVICE_H
# define SERVICE_H

# include <string>
# include <vector>
# include <iostream>
# include "server.hpp"
# include "client.hpp"

// create a client class, which would have its request and respons objects, also status code, ...

class Service
{
	private:

	public:
		std::vector<Server> servers;
		std::map<int, Client> clients;
		Service();
		Service(const Service& other);
		Service& operator=(const Service& other);
		~Service();

		void poll_service();
		void	handle_connection(std::vector<struct pollfd> &poll_fds, const size_t& i);
		void	handle_disconnection(std::vector<struct pollfd> &poll_fds, const size_t& i);
};

// struct pollfd {
//     int fd;        // File descriptor to monitor
//     short events;  // What events you want to know about (POLLIN, POLLOUT)
//     short revents; // What events actually happened (filled by poll)
// };

#endif
