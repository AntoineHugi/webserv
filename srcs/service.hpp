#ifndef SERVICE_H
# define SERVICE_H

# include <string>
# include <vector>
# include "server.hpp"
# include "client.hpp"

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
};

// struct pollfd {
//     int fd;        // File descriptor to monitor
//     short events;  // What events you want to know about (POLLIN, POLLOUT)
//     short revents; // What events actually happened (filled by poll)
// };

#endif
