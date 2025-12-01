#include "service.hpp"

void Service::file_handler(int fd)
{
	// int fd = cgi_fd_for_cgi(fds["poll_fds"][i].fd, fds["files_fd"]);
	std::cout << "file triggered again, fd : " << fd << std::endl;

	Client &client = *(this->files_fds[fd]);
	//std::cout << client._request._uri << std::endl;
	client.update_last_interaction();

	char buffer[BUFFER_SIZE];
	ssize_t bytes;
	std::string body = client._response.get_body();
	bytes = read(fd, buffer, sizeof(buffer));
	std::cout << "bytes read : " << bytes << std::endl;
	if (bytes == -1)
	{
		if (DEBUG)
			std::cout << "Error reading request target" << std::endl;
		client.set_status_code(500);
		client.set_create_response();
		remove_fd(fd);
	}
	else if (bytes == 0)
	{
		std::cout << "finished reading" << std::endl;
		Method::determine_content_type(client, client._request._filepath);
		client.set_status_code(200);
		client.set_create_response();
		remove_fd(fd);
	}
	else
		client._response.set_body(client._response.get_body().append(buffer, bytes));
	return;
}