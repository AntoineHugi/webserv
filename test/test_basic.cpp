#include "test_framework.hpp"

// ============================================================================
// CATEGORY: BASIC CONNECTIVITY & SIMPLE REQUESTS
// ============================================================================

void test_basic_connection(const TestConfig &config, TestStats &stats)
{
	print_test("Basic Connection");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_fail();
		print_fail("Could not connect to server");
		return;
	}

	stats.add_pass();
	print_pass("Connected to server");
	close(sock);
}

void test_simple_get(const TestConfig &config, TestStats &stats)
{
	print_test("Simple GET Request");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	assert_status(resp, 200, stats);
	assert_header_exists(resp, "Content-Type", stats);
	assert_header_exists(resp, "Server", stats);

	close(sock);
}

void test_http_version(const TestConfig &config, TestStats &stats)
{
	print_test("HTTP Version in Response");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	if (resp.status_line.find("HTTP/1.1") == 0)
	{
		stats.add_pass();
		print_pass("HTTP/1.1 version");
	}
	else
	{
		stats.add_fail();
		print_fail("Invalid HTTP version: " + resp.status_line);
	}

	close(sock);
}

void test_required_headers(const TestConfig &config, TestStats &stats)
{
	print_test("Required Response Headers");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	// Check for required headers
	assert_header_exists(resp, "Date", stats);
	assert_header_exists(resp, "Server", stats);
	assert_header_exists(resp, "Content-Length", stats);

	close(sock);
}

void test_per_route_method_limits(const TestConfig &config, TestStats &stats)
{
	print_test("Per-Route Method Restrictions");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0)
	{
		stats.add_skip();
		return;
	}

	std::map<std::string, std::string> headers;

	std::string raw_response = send_request(sock, "POST", "/about", headers);

	HttpResponse resp = parse_response(raw_response);

	assert_status(resp, 405, stats);

	close(sock);
}

// ============================================================================
// CATEGORY RUNNER
// ============================================================================

void run_basic_tests(const TestConfig &config, TestStats &stats)
{
	print_category("BASIC CONNECTIVITY & REQUESTS");

	test_basic_connection(config, stats);
	test_simple_get(config, stats);
	test_http_version(config, stats);
	test_required_headers(config, stats);
	test_per_route_method_limits(config, stats);
}
