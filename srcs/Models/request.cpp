#include "request.hpp"
#include <sstream>
#include <string>
#include <cstdlib>

Request::Request():
	// _reading_lenght(80),
	_request_data(""),
	_header(""),
	_body(""),
	_header_kv(),
	_content_length(0),
	_fullPathURI(""),
	_root(""),
	_isDirectory(false),
	_stat() {}

Request::Request(const Request& other)
{
	// _reading_lenght = other._reading_lenght;
	_request_data = other._request_data;
	_header = other._header;
	_body = other._body;
	_content_length = other._content_length;
	_header_kv = other._header_kv;
	_fullPathURI = other._fullPathURI;
	_root = other._root;
	_isDirectory = other._isDirectory;
	_stat = other._stat;

}

Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		// _reading_lenght = other._reading_lenght;
		_request_data = other._request_data;
		_header = other._header;
		_body = other._body;
		_content_length = other._content_length;
		_header_kv = other._header_kv;
		_fullPathURI = other._fullPathURI;
		_root = other._root;
		_isDirectory = other._isDirectory;
		_stat = other._stat;
	}
	return (*this);
}

Request::~Request() {}

void Request::flush_request_data()
{
	this->_request_data.clear();
	this->_header.clear();
	this->_body.clear();
	this->_content_length = 0;
	this->_header_kv.clear();
}

void Request::parse_header()
{
	std::cout << "\033[34mParsing header...\n\033[0m" << std::endl;
	std::cout << this->_request_data << std::endl;
	std::string line;
	std::istringstream stream(this->_header);

	std::getline(stream, line);
	size_t first_space = line.find(" ");
	size_t second_space = line.find(" ", first_space + 1);

	if (first_space != std::string::npos && second_space != std::string::npos) {
			this->_header_kv["Method"] = line.substr(0, first_space);
			this->_header_kv["URI"] = line.substr(first_space + 1, second_space - first_space - 1);
			this->_header_kv["Version"] = line.substr(second_space + 1);
			if (!this->_header_kv["Version"].empty() && this->_header_kv["Version"].substr(this->_header_kv["Version"].size()-1) == "\r")
				this->_header_kv["Version"].erase(this->_header_kv["Version"].size() - 1);
		}

	while (std::getline(stream, line)) {
		size_t colon_pos = line.find(":");
		if (colon_pos != std::string::npos) {
				std::string key = line.substr(0, colon_pos);
				key.erase(key.find_last_not_of(" ") + 1);
				std::string value = line.substr(colon_pos + 1, line.size() - 1);
				value.erase(0, value.find_first_not_of(" "));
				if (!value.empty() && value.substr(value.size()-1) == "\r")
					value.erase(value.size() - 1);
				this->_header_kv[key] = value;
		}
	}

	char *endptr;
	this->_content_length = this->_header_kv["Content-Length"].empty() ? 0 : std::strtol(this->_header_kv["Content-Length"].c_str(), &endptr, 10);

	std::map<std::string, std::string>::iterator it;
	for (it = this->_header_kv.begin(); it != this->_header_kv.end(); ++it) {
			std::cout << "'" << it->first << "' : '" << it->second << "'" << std::endl;
	}
}


void Request::parse_body()
{
	// TODO: parse Body to _body_parsed string. Specially if data is chunked
	std::cout << "\033[36mParsing body...\n\033[0m" << std::endl;
	std::cout << "\033[36m    Body: " << this->_body << "\n\033[0m" << std::endl;


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


int Request::http_requirements_met()
{
	if (this->_header_kv["Method"] != "GET" &&
		this->_header_kv["Method"] != "POST" &&
		this->_header_kv["Method"] != "DELETE" &&
		this->_header_kv["Method"] != "PUT")
	{
		return 400;
	}

	if (this->_header_kv["Version"] != "HTTP/1.1" &&
		this->_header_kv["Version"] != "HTTP/1.0")
	{
		return 505;
	}
	return 200;
}
bool Request::http_can_have_body()
{
	if (this->_header_kv["Method"] != "POST" &&
		this->_header_kv["Method"] != "PATCH" &&
		this->_header_kv["Method"] != "PUT")
	{
		return false;
	}
	return true;
}
