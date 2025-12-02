#ifndef SERVICE_H
#define SERVICE_H

#include <string>
#include <vector>
#include <iostream>
#include <iostream>
#include <sys/socket.h>
#include <cerrno>
#include <poll.h>
#include <cstring>
#include <stdio.h>
#include <errno.h>

#include "debugPrinting.hpp"
#include "../Models/server.hpp"
#include "../Models/client.hpp"
#include "../Models/CGIProcess.hpp"

extern bool DEBUG;
extern const size_t BUFFER_SIZE;
extern int CLIENT_TIMEOUT_MS;

class Service
{
private:
	public:
		std::map<std::string, std::vector<struct pollfd> > fds;
		std::vector<Server> servers;
		std::map<int, Client> clients;
		std::map<int, CGIProcess *> cgi_processes;
		std::map<int, Client *> files_fds;
		Service();
		Service(const Service &other);
		Service &operator=(const Service &other);
		~Service();

		void poll_service();
		void service_reading(std::vector<struct pollfd> &poll_fds, int i);
		void service_processing(std::vector<struct pollfd> &poll_fds, int i);
		int service_writing(std::vector<struct pollfd> &poll_fds, int i);

		void handle_connection(std::vector<struct pollfd> &poll_fds, const size_t &i);
		void handle_disconnection(std::vector<struct pollfd> &poll_fds, const size_t &i);
		void handle_disconnection_by_fd(int fd);
		void set_polls();
		int server_fd_for_new_client(int fd, std::vector<struct pollfd> &fds_vector);
		int cgi_fd_for_cgi(int fd, std::vector<struct pollfd> &fds_vector);

		void file_handler(int i);
		void cgi_handler(int i);
		void remove_fd(int fd);
		void setup_cgi_request(int i);
		void add_client_to_polls(std::map<int, Client> &clients, int fd, Server &server);
		void add_poll_to_vectors(int fd, int events, std::string additional_poll);
};

void handle_shutdown(int sig);
int find_fd_index_in_vector(int fd, std::vector<struct pollfd> &fds_vector);

/* struct pollfd {
	 int fd;        // File descriptor to monitor
	 short events;  // What events you want to know about (POLLIN, POLLOUT)
	 short revents; // What events actually happened (filled by poll)
 }; */

#endif
