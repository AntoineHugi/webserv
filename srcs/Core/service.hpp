#ifndef SERVICE_H
# define SERVICE_H

# include <string>
# include <vector>
# include <iostream>
# include "../Models/server.hpp"
# include "../Models/client.hpp"
# include "../Models/CGIProcess.hpp"
# include "../Functions/workCGI.hpp"

class Service
{
	private:

	public:
		std::map< std::string, std::vector<struct pollfd> > fds;
		std::vector<Server> servers;
		std::map<int, Client> clients;
		std::map<int, CGIProcess> cgi_processes;
		Service();
		Service(const Service& other);
		Service& operator=(const Service& other);
		~Service();

		void	poll_service();
		void	service_reading(std::vector<struct pollfd> &poll_fds, int i);
		void	service_processing(std::vector<struct pollfd> &poll_fds, int i);
		int	service_writing(std::vector<struct pollfd> &poll_fds, int i);

		void	handle_connection(std::vector<struct pollfd> &poll_fds, const size_t& i);
		void	handle_disconnection(std::vector<struct pollfd> &poll_fds, const size_t& i);
		void	set_polls();
		int		server_fd_for_new_client(int fd, std::vector<struct pollfd> &fds_vector);
		int		cgi_fd_for_cgi(int fd, std::vector<struct pollfd> &fds_vector);

		void	cgi_handler(int i);
		// void	new_client_handler(int fd);
		void	client_handler();
		void	setup_cgi_request(int i);
		CGIProcess	find_cgi_for_this_client(int i);
};

void handle_shutdown(int sig);

// struct pollfd {
//     int fd;        // File descriptor to monitor
//     short events;  // What events you want to know about (POLLIN, POLLOUT)
//     short revents; // What events actually happened (filled by poll)
// };

#endif
