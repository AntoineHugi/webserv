#include "requestUtils.hpp"

std::string get_reason_phrase(int status_code)
{
	std::string response;

	if (status_code / 100 == 2)
	{
		switch (status_code){
			case 200:
				response += "OK\r\n";
				break;
			case 201:
				response += "Created\r\n";
				break;
			case 204:
				response += "No Content\r\n";
				break;
			default:
				response += "Internal Server Error\r\n";
				break;
		}
	}
	else if (status_code / 100 == 3)
	{
		switch (status_code){
			case 301:
				response += "Moved Permanently\r\n";
				break;
			case 302:
				response += "Found\r\n";
				break;
			case 304:
				response += "Not Modified\r\n";
				break;
			default:
				response += "Internal Server Error\r\n";
				break;
		}
	}
	else if (status_code / 100 == 4)
	{
		switch (status_code){
			case 400:
				response += "Bad Request\r\n";
				break;
			case 403:
				response += "Forbidden\r\n";
				break;
			case 404:
				response += "Not Found\r\n";
				break;
			case 405:
				response += "Method Not Allowed\r\n";
				break;
			case 413:
				response += "Payload Too Large\r\n";
				break;
			case 431:
				response += "Request Header Fields Too Large\r\n";
				break;
			default:
				response += "Internal Server Error\r\n";
				break;
		}
	}
	else if (status_code / 100 == 5)
	{
		switch (status_code){
			case 500:
				response += "Internal Server Error\r\n";
				break;
			case 501:
				response += "Not Implemented\r\n";
				break;
			case 503:
				response += "Service Unavailable\r\n";
				break;
			default:
				response += "Internal Server Error\r\n";
				break;
		}
	}
	else
		response += "Internal Server Error\r\n";
	return response;
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
