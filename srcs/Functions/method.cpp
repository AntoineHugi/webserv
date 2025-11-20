#include "method.hpp"

Method::Method() {}

Method::Method(const Method &other)
{
	(void)other;
}

Method &Method::operator=(const Method &other)
{
	(void)other;
	return (*this);
}

Method::~Method() {}

void Method::getDirectory(Client &client, DIR *directory)
{
	if (!directory)
	{
		std::cout << "Error opening requested directory" << std::endl;
		client.set_status_code(500);
		return;
	}
	struct dirent *entry;
	while ((entry = readdir(directory)) != NULL)
	{
		std::string current = client._response.get_body();
		current += std::string(entry->d_name) + '\n';
		client._response.set_body(current);
	}
	closedir(directory);
	client.set_status_code(200);
}

void Method::getFile(Client &client, std::string filepath)
{
	int fd = open(filepath.c_str(), O_RDONLY);
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
		std::cout << "Error reading request target" << std::endl;
		client.set_status_code(500);
		close(fd);
		return;
	}
	client._response.set_body(body);
	close(fd);
	client.set_status_code(200);
}

void Method::handleGet(Client &client)
{
	if (client._request._isDirectory)
	{
		/* looks if there is an index file */
		const std::vector<std::string> &indices = client.get_server()->get_index();
		for (size_t i = 0; i < indices.size(); i++)
		{
			std::string attempt = client._request._fullPathURI + "/" + indices[i];
			if (access(attempt.c_str(), R_OK) == 0)
			{
				getFile(client, attempt);
				return;
			}
		}

		/* if not, then serves the directory list */
		DIR *dir = opendir(client._request._fullPathURI.c_str());
		getDirectory(client, dir);
	}
	else
		Method::getFile(client, client._request._fullPathURI);
	return;
}

void Method::handlePost(Client &client)
{
	//(void)client;
	// need to parse the request body in further detail
	// if (client._request._content_type == "multipart/form-data")
	// 	multipartPost(Client& client)
	// else if (client._request._content_type == "application/x-www-form-urlencoded")
	// 	formPost(Client& client)
	client.set_status_code(200);
	return;
}

void Method::handleDelete(Client &client)
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
