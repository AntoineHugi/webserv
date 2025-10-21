#include "service.hpp"
#include <poll.h>

Service::Service(): servers() {}

Service::Service(const Service& other) 
{
	(void)other;
}

Service& Service::operator=(const Service& other)
{
	(void)other;
	return (*this);
}

Service::~Service() {}

void Service::poll_service()
{
	// Poll array
    std::vector<struct pollfd> poll_fds;
	
	for (size_t i = 0; i < this->servers.size(); i++)
	{
		struct pollfd pfd;
		pfd.fd = this->servers[i].get_sock();
		pfd.events = POLLIN;
		poll_fds.push_back(pfd);
	}

	while(true)
	{
		//check each fd and see if there's any new data in it
		// if there is, decide:
			// is it a server fd? ==> that means a new client has made a connection
				// 'accept' the connection and add the new fd to the pollfd vector
			// else ==> a client that was already connected is sending the data
				// read the information with 'recv' and save the bytes read
				// we need to create a buffer to keep reading in the next iteration
				// thus, might be interesting creating a Client class, or at least a struct with some information of the current reading state
				// after everything was read ==> prepare http answer => answer the client with 'send' ==> close the client's connection and remove it from the fds vector
		// If there is nothing new, just continue the loop

	}
}

