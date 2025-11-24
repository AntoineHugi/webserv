#include "requestUtils.hpp"

std::string get_reason_phrase(int status_code)
{
	switch (status_code)
	{
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 204:
		return "No Content";
	case 301:
		return "Moved Permanently";
	case 302:
		return "Found";
	case 304:
		return "Not Modified";
	case 400:
		return "Bad Request";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 413:
		return "Payload Too Large";
	case 431:
		return "Request Header Fields Too Large";
	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 503:
		return "Service Unavailable";
	case 505:
		return "Service Unavailable";
	}
	return "Internal Server Error";
}

// void parse_application_json(Client &client)
// {

// }

// void parse_application_x_www_form_urlencoded(Client &client)
// {

// }

// void parse_multipart_form_data(Client &client)
// {

// }

// void parse_img(Client &client)
// {

// }

// void parse_text_plain(Client &client)
// {

// }
