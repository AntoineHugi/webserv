#include "test_framework.hpp"
#include <unistd.h>

// ============================================================================
// CATEGORY: ERROR CODES
// ============================================================================

void test_403_forbidden(const TestConfig &config, TestStats &stats)
{
	print_test("403 Forbidden");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/forbidden_folder", headers);
	HttpResponse resp = parse_response(response);

	assert_status(resp, 403, stats);

	close(sock);
}

void test_404_not_found(const TestConfig &config, TestStats &stats)
{
	print_test("404 Not Found");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/nonexistent_file_12345.html", headers);
	HttpResponse resp = parse_response(response);

	assert_status(resp, 404, stats);

	close(sock);
}

void test_405_method_not_allowed(const TestConfig &config, TestStats &stats)
{
	print_test("405 Method Not Allowed");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "POST", "/", headers);
	HttpResponse resp = parse_response(response);

	assert_status(resp, 405, stats);

	close(sock);
}

void test_413_payload_too_large(const TestConfig &config, TestStats &stats)
{
	print_test("413 Payload Too Large");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string big_body(1024 * 1024, 'A');

	std::map<std::string, std::string> headers;

	/*std::ostringstream oss;
	oss << big_body.size();
	headers["Content-Length"] = oss.str();*/

	std::string response = send_request(sock, "POST", "/upload-list", headers, big_body);
	HttpResponse resp = parse_response(response);

	assert_status(resp, 413, stats);

	close(sock);
}

void test_431_request_header_too_large(const TestConfig &config, TestStats &stats)
{
	print_test("431 Request Header Fields Too Large");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	// Oversized header
	std::map<std::string, std::string> headers;
	headers["X-Big-Header"] = std::string(50 * 1024, 'X'); // 50 KB header

	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	assert_status(resp, 431, stats);

	close(sock);
}

void test_505_http_version_not_supported(const TestConfig &config, TestStats &stats)
{
	print_test("505 HTTP Version Not Supported");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string raw_request =
	    "GET / HTTP/3.0\r\n"
	    "Host: " +
	    config.server_host + "\r\n"
				 "\r\n";

	if (send(sock, raw_request.c_str(), raw_request.size(), 0) < 0)
	{
		stats.add_skip();
		print_skip("Send failed");
		close(sock);
		return;
	}

	std::string response;
	char buffer[4096];

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

	while (true)
	{
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0)
		{
			break;
		}
		buffer[received] = '\0';
		response.append(buffer, received);
	}

	HttpResponse resp = parse_response(response);

	assert_status(resp, 505, stats);

	close(sock);
}

void run_error_tests(const TestConfig &config, TestStats &stats)
{
	print_category("ERROR CODES");

	test_403_forbidden(config, stats);
	test_404_not_found(config, stats);
	test_405_method_not_allowed(config, stats);
	test_413_payload_too_large(config, stats);
	test_431_request_header_too_large(config, stats);
	test_505_http_version_not_supported(config, stats);
}
