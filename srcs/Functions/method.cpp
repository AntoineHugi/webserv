#include "method.hpp"

Method::Method() {}

Method::Method(const Method& other)
{
	(void)other;
}

Method& Method::operator=(const Method& other)
{
	(void)other;
	return (*this);
}

Method::~Method() {}

void	Method::handleGet(Client& client)
{
	if (client._request._isDirectory)
	{
		DIR* dir = opendir(client._request._fullPathURI.c_str());
		if (!dir)
		{
			std::cout << "Error opening requested directory" << std::endl;
			client.set_status_code(500);
			return;
		}
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL)
		{
			std::string current = client._response.get_body();
			current += std::string(entry->d_name) + '\n';
			client._response.set_body(current); 
		}
		closedir(dir);
		client.set_status_code(200);
		return;
	}
	else
	{
		int fd = open(client._request._fullPathURI.c_str(), O_RDONLY);
		if (fd == -1)
		{
			std::cout << "Error opening requested file" << std::endl;
			client.set_status_code(500);
			return;
		}
		char buffer[1024];
		ssize_t bytes;
		std::string body = client._response.get_body();
		while ((bytes = read(fd, buffer, sizeof(buffer))) > 0)
			body.append(buffer, bytes);
		if (bytes == -1)
		{
			std::cerr << "Error reading request target" << std::endl;
			client.set_status_code(500);
			close(fd);
			return;
		}
		client._response.set_body(body);
		close(fd);
		client.set_status_code(200);
	}
}

void	Method::handlePost(Client& client)
{
	(void)client;
	// need to parse the request body in further detail
	// if (client._request._content_type == "multipart/form-data")
	// 	multipartPost(Client& client)
	// else if (client._request._content_type == "application/x-www-form-urlencoded")
	// 	formPost(Client& client)
	return;
}


void	Method::handleDelete(Client& client)
{
	if (S_ISREG(client._request._stat.st_mode))
	{
		if (std::remove(client._request._fullPathURI.c_str()) != 0)
		{
			if (errno == ENOENT)
				client.set_status_code(404);
			else
				client.set_status_code(500);
		}
		else
			client.set_status_code(204);
	}
	else
		client.set_status_code(500);
}
