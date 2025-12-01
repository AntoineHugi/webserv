#include "test_framework.hpp"

// ============================================================================
// COLOR CONSTANTS DEFINITIONS
// ============================================================================

namespace Color
{
	const char *const RESET = "\033[0m";
	const char *const RED = "\033[31m";
	const char *const GREEN = "\033[32m";
	const char *const YELLOW = "\033[33m";
	const char *const BLUE = "\033[34m";
	const char *const MAGENTA = "\033[35m";
	const char *const CYAN = "\033[36m";
	const char *const BOLD = "\033[1m";
}

// ============================================================================
// HELPER FUNCTIONS (C++98 Compatible)
// ============================================================================

static std::string int_to_string(int value)
{
	std::ostringstream ss;
	ss << value;
	return ss.str();
}

// ============================================================================
// OUTPUT FUNCTIONS
// ============================================================================

void print_header(const std::string &text)
{
	std::cout << Color::CYAN << Color::BOLD << "\n========================================" << std::endl;
	std::cout << "  " << text << std::endl;
	std::cout << "========================================" << Color::RESET << std::endl;
}

void print_category(const std::string &text)
{
	std::cout << Color::BLUE << Color::BOLD << "\n[" << text << "]" << Color::RESET << std::endl;
	std::cout << Color::BLUE << "==========================================================" << Color::RESET << std::endl;
}

void print_test(const std::string &text)
{
	std::cout << Color::MAGENTA << "TEST: " << text << Color::RESET << std::endl;
}

void print_pass(const std::string &text)
{
	std::cout << Color::GREEN << "✓ PASS: " << Color::RESET << text << std::endl;
}

void print_fail(const std::string &text)
{
	std::cout << Color::RED << "✗ FAIL: " << Color::RESET << text << std::endl;
}

void print_skip(const std::string &text)
{
	std::cout << Color::YELLOW << "○ SKIP: " << Color::RESET << text << std::endl;
}

void print_info(const std::string &text)
{
	std::cout << Color::CYAN << "ℹ INFO: " << Color::RESET << text << std::endl;
}

void print_debug(const std::string &text)
{
	std::cout << Color::YELLOW << "DEBUG: " << Color::RESET << text << std::endl;
}

// ============================================================================
// NETWORK FUNCTIONS
// ============================================================================

int connect_to_server(const char *host, int port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		return -1;
	}

	struct sockaddr_in server_addr;
	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	inet_pton(AF_INET, host, &server_addr.sin_addr);

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		close(sock);
		return -1;
	}

	return sock;
}

std::string send_raw_request(int sock, const std::string &raw_request)
{
	// Send the raw request string
	ssize_t sent = send(sock, raw_request.c_str(), raw_request.size(), 0);
	if (sent < 0)
	{
		return "";
	}

	// Receive response
	std::string response;
	char buffer[4096];

	// Set receive timeout
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	size_t header_end_pos = std::string::npos;
	size_t content_length = 0;
	bool has_content_length = false;

	while (true)
	{
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			break;
		}
		else if (received == 0)
		{
			break;
		}

		buffer[received] = '\0';
		response.append(buffer, received);

		// Find end of headers
		if (header_end_pos == std::string::npos)
			header_end_pos = response.find("\r\n\r\n");

		// Parse Content-Length once headers are complete
		if (header_end_pos != std::string::npos && !has_content_length)
		{
			size_t cl_pos = response.find("Content-Length:");
			if (cl_pos != std::string::npos && cl_pos < header_end_pos)
			{
				size_t value_start = response.find_first_not_of(" \t", cl_pos + 15);
				size_t value_end = response.find("\r\n", value_start);
				if (value_start != std::string::npos && value_end != std::string::npos)
				{
					std::string cl_value = response.substr(value_start, value_end - value_start);
					content_length = atoi(cl_value.c_str());
					has_content_length = true;
				}
			}
			else
			{
				has_content_length = true;
				content_length = 0;
			}
		}

		// Check if we have received the full body
		if (has_content_length)
		{
			size_t body_start = header_end_pos + 4;
			size_t body_received = (response.size() > body_start) ? (response.size() - body_start) : 0;
			if (body_received >= content_length)
				break;
		}
	}

	return response;
}

std::string send_request(int sock, const std::string &method, const std::string &path,
			 const std::map<std::string, std::string> &headers,
			 const std::string &body)
{
	// Build request
	std::ostringstream request;
	request << method << " " << path << " HTTP/1.1\r\n";
	request << "Host: localhost:8080\r\n";

	// Add custom headers
	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
	     it != headers.end(); ++it)
	{
		request << it->first << ": " << it->second << "\r\n";
	}

	if (!body.empty())
	{
		request << "Content-Length: " << body.size() << "\r\n";
	}

	request << "\r\n";

	if (!body.empty())
	{
		request << body;
	}

	std::string req_str = request.str();

	// Send request
	ssize_t sent = send(sock, req_str.c_str(), req_str.size(), 0);
	if (sent < 0)
	{
		return "";
	}

	// Receive response with proper chunked handling
	std::string response;
	char buffer[4096];

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	size_t header_end_pos = std::string::npos;
	size_t content_length = 0;
	bool has_content_length = false;

	while (true)
	{
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				break;
			}
			break;
		}
		else if (received == 0)
		{
			break;
		}

		buffer[received] = '\0';
		response.append(buffer, received);

		// Check if we have complete headers
		if (header_end_pos == std::string::npos)
		{
			header_end_pos = response.find("\r\n\r\n");
		}

		// Once we have headers, parse Content-Length
		if (header_end_pos != std::string::npos && !has_content_length)
		{
			size_t cl_pos = response.find("Content-Length:");
			if (cl_pos != std::string::npos && cl_pos < header_end_pos)
			{
				size_t value_start = response.find_first_not_of(" \t", cl_pos + 15);
				size_t value_end = response.find("\r\n", value_start);
				if (value_start != std::string::npos && value_end != std::string::npos)
				{
					std::string cl_value = response.substr(value_start, value_end - value_start);
					content_length = atoi(cl_value.c_str());
					has_content_length = true;
				}
			}
			else
			{
				has_content_length = true;
				content_length = 0;
			}
		}

		// Check if we have complete response (headers + body)
		if (has_content_length)
		{
			size_t body_start = header_end_pos + 4;
			size_t body_received = (response.size() > body_start) ? (response.size() - body_start) : 0;
			if (body_received >= content_length)
			{
				break;
			}
		}
	}

	return response;
}

HttpResponse parse_response(const std::string &response)
{
	HttpResponse resp;
	resp.status_code = 0;
	resp.raw_response = response;

	size_t header_end = response.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return resp;

	std::string header_section = response.substr(0, header_end);
	resp.body = response.substr(header_end + 4);

	std::istringstream stream(header_section);
	std::string line;

	// Parse status line
	if (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		resp.status_line = line;

		size_t first_space = line.find(' ');
		if (first_space != std::string::npos)
		{
			size_t second_space = line.find(' ', first_space + 1);
			if (second_space != std::string::npos)
			{
				std::string code = line.substr(first_space + 1, second_space - first_space - 1);
				resp.status_code = atoi(code.c_str());
			}
		}
	}

	// Parse headers
	while (std::getline(stream, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		size_t colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);

			// Trim whitespace
			size_t first_char = value.find_first_not_of(" \t\r\n");
			size_t last_char = value.find_last_not_of(" \t\r\n");

			if (first_char != std::string::npos && last_char != std::string::npos)
			{
				value = value.substr(first_char, last_char - first_char + 1);
			}
			else
			{
				value = "";
			}

			resp.headers[key] = value;
		}
	}

	return resp;
}

// ============================================================================
// ASSERTION FUNCTIONS
// ============================================================================

bool assert_status(const HttpResponse &resp, int expected_code, TestStats &stats)
{
	if (resp.status_code == expected_code)
	{
		stats.add_pass();
		print_pass("Status " + int_to_string(expected_code));
		return true;
	}
	else
	{
		stats.add_fail();
		print_fail("Expected " + int_to_string(expected_code) + ", got " + int_to_string(resp.status_code));
		return false;
	}
}

void assert_status_code(int actual, int expected, TestStats &stats)
{
	if (actual == expected)
	{
		stats.add_pass();
		print_pass("Status " + int_to_string(actual));
	}
	else
	{
		stats.add_fail();
		print_fail(
			"Expected " + int_to_string(expected) +
			", got " + int_to_string(actual)
		);
	}
}

bool assert_header(const HttpResponse &resp, const std::string &header,
		   const std::string &expected_value, TestStats &stats)
{
	std::map<std::string, std::string>::const_iterator it = resp.headers.find(header);
	if (it != resp.headers.end() && it->second == expected_value)
	{
		stats.add_pass();
		print_pass(header + ": " + expected_value);
		return true;
	}
	else
	{
		stats.add_fail();
		std::string actual = (it != resp.headers.end()) ? it->second : "<not present>";
		print_fail(header + " expected '" + expected_value + "', got '" + actual + "'");
		return false;
	}
}

bool assert_header_exists(const HttpResponse &resp, const std::string &header, TestStats &stats)
{
	if (resp.headers.find(header) != resp.headers.end())
	{
		stats.add_pass();
		print_pass(header + " header present");
		return true;
	}
	else
	{
		stats.add_fail();
		print_fail(header + " header missing");
		return false;
	}
}

bool assert_body_contains(const HttpResponse &resp, const std::string &text, TestStats &stats)
{
	if (resp.body.find(text) != std::string::npos)
	{
		stats.add_pass();
		print_pass("Body contains '" + text + "'");
		return true;
	}
	else
	{
		stats.add_fail();
		print_fail("Body missing '" + text + "'");
		return false;
	}
}

void assert_body_exists(const HttpResponse &resp, TestStats &stats)
{
	if (!resp.body.empty())
	{
		stats.add_pass();
		print_pass("Response body exists");
	}
	else
	{
		stats.add_fail();
		print_fail("Response body is missing or empty");
	}
}

bool assert_connection_closed(int sock, TestStats &stats)
{
	char buffer[1];
	ssize_t result = recv(sock, buffer, 1, MSG_PEEK);
	if (result == 0)
	{
		stats.add_pass();
		print_pass("Connection closed by server");
		return true;
	}
	else
	{
		stats.add_fail();
		print_fail("Connection still open");
		return false;
	}
}
