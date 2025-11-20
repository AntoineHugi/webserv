#ifndef SERVICE_H
# define SERVICE_H

# include <string>
# include <vector>
# include <iostream>
# include "../Models/server.hpp"
# include "../Models/client.hpp"
# include "../Functions/workCGI.hpp"

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

		void	poll_service();
		int	service_reading(std::vector<struct pollfd> &poll_fds, int i);
		int	service_processing(std::vector<struct pollfd> &poll_fds, int i);
		int	service_writing(std::vector<struct pollfd> &poll_fds, int i);

		void	handle_connection(std::vector<struct pollfd> &poll_fds, const size_t& i);
		void	handle_disconnection(std::vector<struct pollfd> &poll_fds, const size_t& i);
};

void handle_shutdown(int sig);

// struct pollfd {
//     int fd;        // File descriptor to monitor
//     short events;  // What events you want to know about (POLLIN, POLLOUT)
//     short revents; // What events actually happened (filled by poll)
// };

#endif
