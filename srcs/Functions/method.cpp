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

void Method::get_directory(Client &client, DIR *directory)
{
	if (!directory)
	{
		if (DEBUG)
			std::cout << "Error opening requested directory" << std::endl;
		client.set_status_code(500);
		return;
	}
	std::string html;
	html += "<!DOCTYPE html>\n";
	html += "<html><head><title>Index of ";
	html += client._request._uri;
	html += "</title></head><body>\n";
	html += "<ul>\n";

	struct dirent *entry;
	while ((entry = readdir(directory)) != NULL)
	{
		std::string name(entry->d_name);

		if (name == "." || name == "..")
			continue;
		html += "  <li>";
		html += name;
		html += "</li>\n";
	}

	html += "</ul>\n";
	html += "</body></html>\n";

	closedir(directory);

	client._response.set_body(html);
	client._response.set_content_type("text/html");
	client.set_status_code(200);
}

void Method::determine_content_type(Client &client, std::string filepath)
{
	size_t dot = filepath.find_last_of('.');
	std::string content_type;
	if (dot == std::string::npos)
	{
		client._response.set_content_type("application/octet-stream");
		return;
	}
	else
	{
		std::string ext = filepath.substr(dot + 1);
		for (size_t i = 0; i < ext.size(); i++)
			ext[i] = std::tolower(ext[i]);
		if (ext == "html" || ext == "htm")
		{
			content_type = "text/html";
		}
		else if (ext == "txt")
		{
			content_type = "text/plain";
		}
		else if (ext == "json")
		{
			content_type = "application/json";
		}
		else if (ext == "png")
		{
			content_type = "image/png";
		}
		else if (ext == "jpg" || ext == "jpeg")
		{
			content_type = "image/jpeg";
		}
		else if (ext == "webp")
		{
			content_type = "image/webp";
		}
		else if (ext == "gif")
		{
			content_type = "image/gif";
		}
		else if (ext == "pdf")
		{
			content_type = "application/pdf";
		}
		else if (ext == "csv")
		{
			content_type = "text/csv";
		}
		else if (ext == "js")
		{
			content_type = "application/javascript";
		}
		else if (ext == "py")
		{
			content_type = "text/x-python";
		}
		else
		{
			content_type = "application/octet-stream";
		}
	}
	client._response.set_content_type(content_type);
	return;
}

void Method::get_file(Client &client, std::string filepath)
{
	int fd = open(filepath.c_str(), O_RDONLY);
	if (fd == -1)
	{
		if (DEBUG)
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
		if (DEBUG)
			std::cout << "Error reading request target" << std::endl;
		client.set_status_code(500);
		close(fd);
		return;
	}
	client._response.set_body(body);
	close(fd);
	determine_content_type(client, filepath);
	client.set_status_code(200);
}

void Method::handle_get(Client &client)
{
	if (client._request._isDirectory)
	{
		/* looks if there is an index file */
		const std::vector<std::string> &indices = client._request._index;
		for (size_t i = 0; i < indices.size(); i++)
		{
			std::string attempt = client._request._fullPathURI + "/" + indices[i];
			if (access(attempt.c_str(), R_OK) == 0)
			{
				get_file(client, attempt);
				return;
			}
		}
		/* if not, then serves the directory list if autoindex is on */
		if (client._request._autoindex)
		{
			DIR *dir = opendir(client._request._fullPathURI.c_str());
			get_directory(client, dir);
		}
		else
		{
			client.set_status_code(404);
			return;
		}
	}
	else
		Method::get_file(client, client._request._fullPathURI);
	return;
}

int Method::save_uploaded_files(Client& client, std::vector<MultiPart> &parts, const std::string &upload_directory)
{
	for (size_t i = 0; i < parts.size(); ++i)
	{
		if (parts[i].get_file_name().empty())
			continue;
		std::string path = upload_directory + "/" + parts[i].get_file_name();
		std::ofstream out(path.c_str(), std::ios::binary);
		if (!out.is_open())
		{
			if (DEBUG)
				std::cout << "Failed to open file for writing: " << path << std::endl;
			return (1);
		}
		const std::vector<char> &data = parts[i].get_file_data();
		out.write(data.data(), data.size());
		out.close();
		if (DEBUG)
			std::cout << "Saved file: " << path << " to disc." << std::endl;
		client._response.set_location(path);
	}
	return (0);
}

std::string Method::double_quote_handling(const std::string &input)
{
	std::string modified = input;
	size_t pos = 0;

	while ((pos = modified.find('\"', pos)) != std::string::npos)
	{
		modified.insert(pos, "\"");
		pos += 2;
	}

	return ("\"" + modified + "\"");
}

int Method::input_data(Client &client)
{
	size_t pos = client._request._fullPathURI.find_last_of('/');
	if (pos == std::string::npos)
	{
		client.set_status_code(400);
		return (1);
	}
	std::string path = client._request._root + client._request._fullPathURI.substr(pos);
	if (DEBUG)
	{
		std::cout << "initially: " << client._request._fullPathURI << std::endl;
		std::cout << "path to write input: " << path << std::endl;
	}

	std::ofstream out(path.c_str(), std::ios::app);
	if (!out.is_open())
	{
		client.set_status_code(500);
		return (1);
	}

	std::string first = Method::double_quote_handling(client._request._body_kv["firstName"]);
	std::string last = Method::double_quote_handling(client._request._body_kv["lastName"]);

	out << first << "," << last << "\n";
	out.close();

	return 0;
}

void Method::handle_post(Client &client)
{
	if (DEBUG)
		std::cout << "content-type: " << client._request._header_kv["content-type"] << std::endl;
	if (client._request._header_kv["content-type"] == "application/x-www-form-urlencoded")
	{
		if (input_data(client))
			return;
		client.set_status_code(204);
		return;
	}
	else if (client._request._header_kv["content-type"].find("multipart/form-data") != std::string::npos)
	{
		if (save_uploaded_files(client, client._request._multiparts, client._request._fullPathURI) == 1)
		{
			client.set_status_code(500);
			return;
		}
		client.set_status_code(201);
		return;
	}
	else if (client._request._header_kv["content-type"].empty())
	{
		client.set_status_code(200);
		return;
	}
	else
	{
		client.set_status_code(200);
		return;
	}
	return;
}

void Method::handle_delete(Client &client)
{
	if (S_ISREG(client._request._stat.st_mode))
	{
		if (std::remove(client._request._fullPathURI.c_str()) != 0)
			client.set_status_code(500);
		else
			client.set_status_code(204);
	}
	else
		client.set_status_code(500);
}
