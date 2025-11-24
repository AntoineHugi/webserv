#include "request.hpp"

Request::Request() : _request_data(""),
		     _header(""),
		     _method(""),
		     _uri(""),
		     _version(""),
		     _header_kv(),
		     _content_length(0),
		     _fullPathURI(""),
		     _root(""),
		     _isDirectory(false),
		     _stat(),
		     _isCGI(false),
		     _body(""),
		     _body_kv(),
		     _multiparts(),
		     _body_data()
{
}

Request::Request(const Request &other)
{
	_request_data = other._request_data;
	_header = other._header;
	_method = other._method;
	_uri = other._uri;
	_version = other._version;
	_header_kv = other._header_kv;
	_content_length = other._content_length;
	_fullPathURI = other._fullPathURI;
	_root = other._root;
	_isDirectory = other._isDirectory;
	_stat = other._stat;
	_isCGI = other._isCGI;
	_body = other._body;
	_body_kv = other._body_kv;
	_multiparts = other._multiparts;
	_body_data = other._body_data;
}

Request &Request::operator=(const Request &other)
{
	if (this != &other)
	{
		_request_data = other._request_data;
		_header = other._header;
		_method = other._method;
		_uri = other._uri;
		_version = other._version;
		_header_kv = other._header_kv;
		_content_length = other._content_length;
		_fullPathURI = other._fullPathURI;
		_root = other._root;
		_isDirectory = other._isDirectory;
		_stat = other._stat;
		_isCGI = other._isCGI;
		_body = other._body;
		_body_kv = other._body_kv;
		_multiparts = other._multiparts;
		_body_data = other._body_data;
	}
	return (*this);
}

Request::~Request() {}

void Request::flush_request_data()
{
	_request_data.clear();
	_header.clear();
	_method.clear();
	_uri.clear();
	_version.clear();
	_header_kv.clear();
	_content_length = 0;
	_fullPathURI.clear();
	_root.clear();
	_isDirectory = false;
	memset(&_stat, 0, sizeof(_stat));
	_isCGI = false;
	_body.clear();
	_body_kv.clear();
	_multiparts.clear();
	_body_data.clear();
}

int Request::http_requirements_met()
{
	if (_method.empty() || _uri.empty() || _version.empty())
	{
		std::cout << "something is empty, _method = " << _method << "; _uri = " << _uri << ";_version = " << _version << std::endl;
		return 505;
	}
	if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
	{
		std::cout << "version is " << _version << std::endl;
		return 505;
	}
	return 200;
}

bool Request::http_can_have_body()
{
	if (_method != "POST" && _method != "PATCH" && _method != "PUT")
		return false;
	return true;
}

int Request::parse_header()
{
	std::cout << "\033[34mParsing header...\n\033[0m" << std::endl;
	while (!_header.empty() && (_header[0] == '\r' || _header[0] == '\n'))
		_header.erase(0, 1);
	std::string line;
	std::istringstream stream(_header);

	std::getline(stream, line);
	size_t first_space = line.find(" ");
	size_t second_space = line.find(" ", first_space + 1);

	if (first_space != std::string::npos && second_space != std::string::npos)
	{
		_method = line.substr(0, first_space);
		_uri = line.substr(first_space + 1, second_space - first_space - 1);
		if (_uri.size() > 2048)
			return (1);
		_version = line.substr(second_space + 1);
		if (!_version.empty() && _version.substr(_version.size() - 1) == "\r")
			_version.erase(_version.size() - 1);
	}
	while (std::getline(stream, line))
	{
		size_t colon_pos = line.find(":");
		if (colon_pos != std::string::npos)
		{
			std::string key = line.substr(0, colon_pos);
			key.erase(key.find_last_not_of(" ") + 1);
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			std::string value = line.substr(colon_pos + 1, line.size() - 1);
			value.erase(0, value.find_first_not_of(" "));

			if (!value.empty() && value.substr(value.size() - 1) == "\r")
				value.erase(value.size() - 1);

			if (!_header_kv.count(key))
				_header_kv[key] = value;
			else
			{
				std::cout << "duplicate keys in header" << std::endl;
				return (1);
			}
		}
	}
	/*
		However, there's a robustness issue with strtol parsing:
		Content-Length: 123abc456  // strtol returns 123, endptr points to "abc456"
		You accept 123 but ignore abc456 (garbage). This is technically invalid HTTP.
	*/
	if (!_header_kv.count("content-length"))
		_content_length = 0;
	else
	{
		char *endptr;
		long val = std::strtol(_header_kv["content-length"].c_str(), &endptr, 10);
		if (*endptr != '\0' && *endptr != '\r')
		{
			std::cout << "Bad formatting: content-length" << std::endl;
			return (1);
		}
		_content_length = val;
	}

	std::map<std::string, std::string>::iterator it;
	for (it = _header_kv.begin(); it != _header_kv.end(); ++it)
	{
		std::cout << "'" << it->first << "' : '" << it->second << "'" << std::endl;
	}

	std::cout << "\033[34mHeader parsed sucessfully! \n\033[0m" << std::endl;
	return (0);
}

std::vector<std::string> Request::tokenise_url_encoded(std::string &str)
{
	std::vector<std::string> tokens;
	std::istringstream iss(str);
	std::string word;

	while (iss >> word)
	{
		std::string current;

		for (size_t i = 0; i < word.size(); ++i)
		{
			if (word[i] == '&' || word[i] == '=')
			{
				if (!current.empty())
				{
					tokens.push_back(current);
					current.clear();
				}
				tokens.push_back(std::string(1, word[i]));
			}
			else
				current += word[i];
		}
		if (!current.empty())
			tokens.push_back(current);
	}
	return (tokens);
}

int Request::parse_url_encoded()
{
	std::vector<std::string> tokens = tokenise_url_encoded(_body);
	for (size_t i = 0; i < tokens.size(); ++i)
	{
		if (tokens[i] == "=" || tokens[i] == "&")
			return (1);
		std::string key = tokens[i];
		i++;
		if (tokens[i] != "=")
			return (1);
		i++;
		if (tokens[i] == "=" || tokens[i] == "&")
			return (1);
		std::string value = tokens[i];
		_body_kv[key] = value;
		i++;
		if (tokens[i] != "&")
			return (1);
	}
	return (0);
}

std::string Request::trimCRLF(const std::string &s)
{
	size_t start = 0;
	size_t end = s.size();

	while (start < end && (s[start] == '\r' || s[start] == '\n'))
		start++;
	while (end > start && (s[end - 1] == '\r' || s[end - 1] == '\n'))
		end--;

	return s.substr(start, end - start);
}

std::vector<MultiPart> Request::generate_multipart(const std::string &boundary)
{
	std::vector<MultiPart> multipart;

	std::string realBoundary = "--" + boundary;
	std::string endBoundary = realBoundary + "--";

	size_t pos = 0;

	while (true)
	{
		size_t start = _body.find(realBoundary, pos);
		if (start == std::string::npos)
			break;
		start += realBoundary.size();
		if (_body.compare(start, 2, "--") == 0)
			break;
		if (_body.compare(start, 2, "--") == 0)
			break;
		if (_body.compare(start, 2, "\r\n") == 0)
			start += 2;
		size_t headerEnd = _body.find("\r\n\r\n", start);
		if (headerEnd == std::string::npos)
			break;
		std::string headerBlock = _body.substr(start, headerEnd - start);
		size_t contentStart = headerEnd + 4;
		size_t nextBoundary = _body.find(realBoundary, contentStart);
		if (nextBoundary == std::string::npos)
			break;
		size_t contentEnd = nextBoundary;
		std::vector<char> rawContent(_body.begin() + contentStart, _body.begin() + contentEnd);
		if (rawContent.size() >= 2 && rawContent[rawContent.size() - 1] == '\n' && rawContent[rawContent.size() - 2] == '\r')
			rawContent.erase(rawContent.end() - 2, rawContent.end());

		MultiPart part;
		std::stringstream hs(headerBlock);
		std::string line;

		while (std::getline(hs, line))
		{
			line = trimCRLF(line);

			if (line.find("Content-Disposition:") == 0)
			{
				size_t namePos = line.find("name=\"");
				if (namePos != std::string::npos)
				{
					namePos += 6;
					size_t endPos = line.find("\"", namePos);
					part.set_name(line.substr(namePos, endPos - namePos));
					std::cout << "part name = " << part.get_name() << std::endl;
				}

				size_t filePos = line.find("filename=\"");
				if (filePos != std::string::npos)
				{
					filePos += 10;
					size_t endPos = line.find("\"", filePos);
					part.set_file_name(line.substr(filePos, endPos - filePos));
					std::cout << "file name = " << part.get_file_name() << std::endl;
				}
			}
			else if (line.find("Content-Type:") == 0)
			{
				size_t sep = line.find(":");
				std::string type = line.substr(sep + 1);
				while (!type.empty() && type[0] == ' ')
					type.erase(0, 1);
				part.set_MIME_type(type);
				std::cout << "file type = " << part.get_MIME_type() << std::endl;
			}
		}
		part.set_file_data(rawContent);
		multipart.push_back(part);
		pos = nextBoundary;
	}
	return multipart;
}

std::string Request::find_boundary(std::string content_type)
{
	size_t pos = content_type.find("boundary");
	if (pos == std::string::npos || content_type.size() < pos + 9)
		return ("");
	std::string boundary = content_type.substr(pos + 9);
	size_t sc = boundary.find(';');
	if (sc != std::string::npos)
		boundary = boundary.substr(0, sc);
	while (!boundary.empty() && (boundary[0] == ' ' || boundary[0] == '='))
		boundary.erase(0, 1);
	return (boundary);
}

int Request::parse_multipart(std::string content_type)
{
	std::string boundary = find_boundary(content_type);
	if (boundary.empty())
		return (1);
	_multiparts = generate_multipart(boundary);
	if (_multiparts.empty())
		return (1);
	return (0);
}

int Request::parse_json()
{
	return (0);
}

int Request::treat_as_raw_body()
{
	std::vector<char> rawContent(_body.begin(), _body.end());
	if (rawContent.size() >= 2 && rawContent[rawContent.size() - 1] == '\n' && rawContent[rawContent.size() - 2] == '\r')
		rawContent.erase(rawContent.end() - 2, rawContent.end());
	_body_data = rawContent;
	return (0);
}

int Request::parse_body()
{
	// TODO: parse Body to _body_parsed string. Specially if data is chunked
	std::cout << "\033[36mParsing body...\n\033[0m" << std::endl;
	std::cout << "\033[36m    Body: " << _body << "\n\033[0m" << std::endl;

	std::string content_type = _header_kv["content-type"];
	if (content_type == "application/x-www-form-urlencoded")
		return (parse_url_encoded());
	else if (content_type.find("multipart/form-data") != std::string::npos)
		return (parse_multipart(content_type));
	else if (content_type == "application/json")
		return (parse_json());
	else
		return (treat_as_raw_body());

	// if(this->_request._header_kv["Content-type"] == "application/json")
	// 	parse_application_json(this);
	// else if(this->_request._header_kv["Content-type"] == "application/x-www-form-urlencoded")
	// 	parse_application_x_www_form_urlencoded(this);
	// else if(this->_request._header_kv["Content-type"].find("multipart/form-data") != std::string::npos)
	// 	parse_multipart_form_data(this);
	// else if(this->_request._header_kv["Content-type"] == "image/png")
	// 	parse_img(this);
	// else
	// {
	// 	if(this->_request._header_kv["Content-type"] != "text/plain")
	// 		std::cout << "Unknown or missing Content-Type. Treating body as plain text." << std::endl;
	// 	parse_text_plain(this);
	// }

	/* WIP */
	/*

	Content-Type: text/html
	Content-Type: multipart/form-data; boundary=----WebKitFormBoundary
	Content-Type: image/png
	Content-Type: text/plain

	//// EXAMPLE 1. **application/json** (JSON data)
	```
				POST /api/items HTTP/1.1
				Host: example.com
				Content-Type: application/json
				Content-Length: 45

				{
					"name": "New Item",
					"price": 19.99
				}
	```

		//// EXAMPLE 2. **application/xml** (XML data)
	```
				POST /api/soap HTTP/1.1
				Host: example.com
				Content-Type: text/xml
				Content-Length: 85

				<?xml version="1.0" encoding="UTF-8"?>
				<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
					<soap:Body>
						<MyRequest>
							<Parameter>Value</Parameter>
						</MyRequest>
					</soap:Body>
				</soap:Envelope>
	```

	//// EXAMPLE 3. **application/x-www-form-urlencoded** (Form data)
	```
				POST /submit-form HTTP/1.1
				Host: example.com
				Content-Type: application/x-www-form-urlencoded
				Content-Length: 38

				name=John+Doe&age=30
	```

	//// EXAMPLE 3.1 **application/x-www-form-urlencoded** (Form data)

	```
				POST /api/search?q=example+search HTTP/1.1
				Host: example.com
				Content-Type: application/x-www-form-urlencoded
				Content-Length: 0
	```
	Like query strings: `key=value` pairs separated by `&`.
	Used by HTML forms by default.

	//// EXAMPLE 4. **text/plain** (Raw text)
	```
				POST /upload HTTP/1.1
				Host: example.com
				Content-Type: text/plain
				Content-Length: 11

				Hello World
	```

	Just plain text.

	//// EXAMPLE 5. **multipart/form-data** (File uploads + form fields)
	```

				POST /upload HTTP/1.1
				Host: example.com
				Content-Type: multipart/form-data; boundary=---011000010111000001110010

				-----011000010111000001110010
				Content-Disposition: form-data; name="file"; filename="example.txt"
				Content-Type: text/plain

				Content of the file goes here.
				-----011000010111000001110010
				Content-Disposition: form-data; name="description"

				This is an example file upload.
				-----011000010111000001110010--

	Complex format for uploading files. Each part has its own headers.


	//// EXAMPLE 6. **chunked transfer encoding**
	```
				POST /api/items HTTP/1.1
				Host: example.com
				Transfer-Encoding: chunked
				Content-Type: application/json

				7
				{
					"name":
				8
					"New Item",
				7
					"price":
				9
					19.99
				0
	```

	//// EXAMPLE 7. **No Content-Type** (Treat as binary or text)
	```
				POST /upload HTTP/1.1
				Host: example.com
				// Content-Type: text/plain
				Content-Length: 11

				Hello World
	```

	If no Content-Type, you can:
	- Treat as plain text
	- Treat as binary data
	- Return error (400 Bad Request)


	*/
}
