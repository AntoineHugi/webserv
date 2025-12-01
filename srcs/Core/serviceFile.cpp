#include "service.hpp"

void Service::file_handler(int fd)
{
	Client &client = *(this->files_fds[fd]);
	client.update_last_interaction();

	char buffer[BUFFER_SIZE];
	ssize_t bytes;
	std::string body = client._response.get_body();
	bytes = read(fd, buffer, sizeof(buffer));
	if (bytes == -1)
	{
		if (DEBUG)
			std::cout << "Error reading request target" << std::endl;
		client.set_status_code(500);
		client.set_process_request();
		remove_fd(fd);
	}
	else if (bytes == 0)
	{
		Method::determine_content_type(client, client._request._filepath);
		if (client.get_status_code() == 0)
			client.set_status_code(200);
		client.set_create_response();
		remove_fd(fd);
	}
	else
		client._response.set_body(client._response.get_body().append(buffer, bytes));
	return;
}