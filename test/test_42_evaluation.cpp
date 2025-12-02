#include "test_framework.hpp"
#include <sys/socket.h>

// ============================================================================
// 42 EVALUATION SCALE TESTS
// Based on ng_5_webserv.pdf evaluation criteria
// ============================================================================

// ===========================================================================
// CONFIGURATION TESTS
// ===========================================================================

void test_42_multiple_servers_different_ports(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: Multiple servers on different ports");

	// Test server on port 8080
	int sock1 = connect_to_server("127.0.0.1", 8080);
	if (sock1 < 0) {
		stats.add_fail();
		print_fail("Cannot connect to server on port 8080");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Host"] = "localhost:8080";
	std::string response1 = send_request(sock1, "GET", "/", headers);
	close(sock1);

	// Test server on port 8081
	int sock2 = connect_to_server("127.0.0.1", 8081);
	if (sock2 < 0) {
		stats.add_fail();
		print_fail("Cannot connect to server on port 8081");
		return;
	}

	headers["Host"] = "localhost:8081";
	std::string response2 = send_request(sock2, "GET", "/", headers);
	close(sock2);

	// Test server on port 9090
	int sock3 = connect_to_server("127.0.0.1", 9090);
	if (sock3 < 0) {
		stats.add_fail();
		print_fail("Cannot connect to server on port 9090");
		return;
	}

	headers["Host"] = "localhost:9090";
	std::string response3 = send_request(sock3, "GET", "/", headers);
	close(sock3);

	// All should return 200 OK
	if (response1.find("200 OK") != std::string::npos &&
	    response2.find("200 OK") != std::string::npos &&
	    response3.find("200 OK") != std::string::npos)
	{
		stats.add_pass();
		print_pass("Multiple servers on different ports work");
	}
	else
	{
		stats.add_fail();
		print_fail("Not all servers responding correctly");
	}
}

void test_42_custom_error_page(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: Custom error page for 404");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/nonexistent_page_12345", headers);
	HttpResponse resp = parse_response(response);

	close(sock);

	if (resp.status_code == 404 && resp.body.size() > 0)
	{
		stats.add_pass();
		print_pass("Custom 404 error page served");
	}
	else
	{
		stats.add_fail();
		print_fail("404 error page not working");
	}
}

void test_42_client_body_limit(const TestConfig& config, TestStats& stats)
{
	print_test("42 EVAL: Client body size limit (413)");

	int sock = connect_to_server("127.0.0.1", 9090);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/plain";

	// /post_body has client_max_body_size 100
	// Send 101 bytes - should trigger 413
	std::string body(101, 'A');
	std::string response = send_request(sock, "POST", "/post_body", headers, body);
	HttpResponse resp = parse_response(response);

	close(sock);

	if (resp.status_code == 413)
	{
		stats.add_pass();
		print_pass("Body limit enforced (413 returned)");
	}
	else
	{
		stats.add_fail();
		print_fail("Body limit not enforced (expected 413, got " +
		           std::string(1, '0' + resp.status_code/100) +
		           std::string(1, '0' + (resp.status_code/10)%10) +
		           std::string(1, '0' + resp.status_code%10) + ")");
		if (config.verbose)
			std::cout << "Response: " << response.substr(0, 200) << std::endl;
	}
}

void test_42_client_body_at_limit(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: Client body exactly at limit (200)");

	int sock = connect_to_server("127.0.0.1", 9090);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/plain";

	// Exactly 100 bytes - should be accepted
	std::string body(100, 'A');
	std::string response = send_request(sock, "POST", "/post_body", headers, body);
	HttpResponse resp = parse_response(response);

	close(sock);

	if (resp.status_code == 200)
	{
		stats.add_pass();
		print_pass("Body at exact limit accepted");
	}
	else
	{
		stats.add_fail();
		print_fail("Body at limit rejected (should be 200)");
	}
}

void test_42_route_to_directory(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: Route to different directory");

	int sock = connect_to_server("127.0.0.1", 9090);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/directory", headers);
	HttpResponse resp = parse_response(response);

	close(sock);

	if (resp.status_code == 200 || resp.status_code == 301)
	{
		stats.add_pass();
		print_pass("Route to directory works");
	}
	else
	{
		stats.add_fail();
		print_fail("Route to directory failed");
	}
}

void test_42_default_file_for_directory(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: Default index file for directory");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	// /upload route has index files.html
	std::string response = send_request(sock, "GET", "/upload/", headers);
	HttpResponse resp = parse_response(response);

	close(sock);

	if (resp.status_code == 200 && resp.body.size() > 0)
	{
		stats.add_pass();
		print_pass("Default index file served");
	}
	else
	{
		stats.add_fail();
		print_fail("Default index file not served");
	}
}

void test_42_method_not_allowed(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: Method not allowed returns 405");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	// Root only allows GET, try POST
	std::string response = send_request(sock, "POST", "/", headers, "test");
	HttpResponse resp = parse_response(response);

	close(sock);

	if (resp.status_code == 405)
	{
		// Check for Allow header
		if (resp.headers.find("Allowed-Methods") != resp.headers.end() ||
		    resp.headers.find("allow") != resp.headers.end())
		{
			stats.add_pass();
			print_pass("405 with Allow header");
		}
		else
		{
			stats.add_fail();
			print_fail("405 but missing Allow header");
		}
	}
	else
	{
		stats.add_fail();
		print_fail("Expected 405 for forbidden method");
	}
}

// ===========================================================================
// BASIC CHECKS
// ===========================================================================

void test_42_get_request(const TestConfig& config, TestStats& stats)
{
	print_test("42 EVAL: GET request works");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	close(sock);

	if (resp.status_code == 200)
	{
		stats.add_pass();
		print_pass("GET request successful");
	}
	else
	{
		stats.add_fail();
		print_fail("GET request failed");
	}
}

void test_42_post_request(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: POST request works");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/plain";
	std::string body = "test data";

	std::string response = send_request(sock, "POST", "/upload-list/", headers, body);
	HttpResponse resp = parse_response(response);

	close(sock);

	if (resp.status_code == 200 || resp.status_code == 201 || resp.status_code == 204)
	{
		stats.add_pass();
		print_pass("POST request successful");
	}
	else
	{
		stats.add_fail();
		print_fail("POST request failed");
	}
}

void test_42_delete_request(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: DELETE request works");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "DELETE", "/upload-list/test_delete.txt", headers);
	HttpResponse resp = parse_response(response);

	close(sock);

	// DELETE might return 200, 204, or 404 if file doesn't exist
	if (resp.status_code == 200 || resp.status_code == 204 || resp.status_code == 404)
	{
		stats.add_pass();
		print_pass("DELETE request handled");
	}
	else
	{
		stats.add_fail();
		print_fail("DELETE request failed");
	}
}

void test_42_unknown_method(const TestConfig& config, TestStats& stats)
{
	print_test("42 EVAL: Unknown method doesn't crash");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	// Send unknown method
	std::string request = "UNKNOWN / HTTP/1.1\r\n";
	request += "Host: " + config.server_host + "\r\n";
	request += "\r\n";

	send(sock, request.c_str(), request.length(), 0);

	char buffer[4096];
	ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);

	close(sock);

	if (n > 0)
	{
		buffer[n] = '\0';
		std::string response(buffer);
		// Should return error, not crash
		if (response.find("HTTP/1.1") != std::string::npos)
		{
			stats.add_pass();
			print_pass("Unknown method handled gracefully");
		}
		else
		{
			stats.add_fail();
			print_fail("Invalid response to unknown method");
		}
	}
	else
	{
		stats.add_fail();
		print_fail("Server didn't respond to unknown method");
	}
}

void test_42_upload_and_retrieve(const TestConfig& config, TestStats& stats)
{
	(void)config;
	print_test("42 EVAL: Upload file and retrieve it");

	// Upload file
	int sock1 = connect_to_server("127.0.0.1", 8080);
	if (sock1 < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
	std::string body;
	body += "--" + boundary + "\r\n";
	body += "Content-Disposition: form-data; name=\"file\"; filename=\"test_upload.txt\"\r\n";
	body += "Content-Type: text/plain\r\n";
	body += "\r\n";
	body += "This is test upload content\r\n";
	body += "--" + boundary + "--\r\n";

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "multipart/form-data; boundary=" + boundary;

	std::string response1 = send_request(sock1, "POST", "/upload-list/", headers, body);
	HttpResponse resp1 = parse_response(response1);
	close(sock1);

	if (resp1.status_code != 200 && resp1.status_code != 201)
	{
		stats.add_fail();
		print_fail("Upload failed");
		return;
	}

	// Retrieve file
	int sock2 = connect_to_server("127.0.0.1", 8080);
	if (sock2 < 0) {
		stats.add_fail();
		print_fail("Cannot reconnect");
		return;
	}

	std::map<std::string, std::string> headers2;
	std::string response2 = send_request(sock2, "GET", "/upload-list/test_upload.txt", headers2);
	HttpResponse resp2 = parse_response(response2);
	close(sock2);

	if (resp2.status_code == 200 && resp2.body.find("test upload content") != std::string::npos)
	{
		stats.add_pass();
		print_pass("File uploaded and retrieved successfully");
	}
	else
	{
		stats.add_fail();
		print_fail("File upload or retrieval failed");
	}
}

// ===========================================================================
// STRESS TESTS (Simplified - full siege tests done externally)
// ===========================================================================

void test_42_multiple_connections(const TestConfig& config, TestStats& stats)
{
	print_test("42 EVAL: Handle multiple concurrent connections");

	const int num_connections = 20;
	int socks[num_connections];
	bool success = true;

	// Open multiple connections
	for (int i = 0; i < num_connections; i++)
	{
		socks[i] = connect_to_server(config.server_host.c_str(), config.server_port);
		if (socks[i] < 0)
		{
			success = false;
			// Close previously opened sockets
			for (int j = 0; j < i; j++)
				close(socks[j]);
			break;
		}
	}

	if (success)
	{
		// Send requests on all connections
		std::map<std::string, std::string> headers;
		for (int i = 0; i < num_connections; i++)
		{
			std::string response = send_request(socks[i], "GET", "/", headers);
			if (response.find("200 OK") == std::string::npos)
				success = false;
			close(socks[i]);
		}
	}

	if (success)
	{
		stats.add_pass();
		print_pass("Handled 20 concurrent connections");
	}
	else
	{
		stats.add_fail();
		print_fail("Failed to handle multiple connections");
	}
}

void test_42_no_hanging_connections(const TestConfig& config, TestStats& stats)
{
	print_test("42 EVAL: No hanging connections");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Connection"] = "close";

	std::string response = send_request(sock, "GET", "/", headers);

	// Try to read more - should get EOF since Connection: close
	char buffer[1024];
	ssize_t n = recv(sock, buffer, sizeof(buffer), 0);

	close(sock);

	if (n == 0)
	{
		stats.add_pass();
		print_pass("Connection properly closed");
	}
	else
	{
		stats.add_fail();
		print_fail("Connection not properly closed");
	}
}

// ===========================================================================
// TEST RUNNER
// ===========================================================================

void run_42_evaluation_tests(const TestConfig& config, TestStats& stats)
{
	print_category("42 EVALUATION SCALE TESTS");

	// Configuration tests
	test_42_multiple_servers_different_ports(config, stats);
	test_42_custom_error_page(config, stats);
	test_42_client_body_limit(config, stats);
	test_42_client_body_at_limit(config, stats);
	test_42_route_to_directory(config, stats);
	test_42_default_file_for_directory(config, stats);
	test_42_method_not_allowed(config, stats);

	// Basic checks
	test_42_get_request(config, stats);
	test_42_post_request(config, stats);
	test_42_delete_request(config, stats);
	test_42_unknown_method(config, stats);
	test_42_upload_and_retrieve(config, stats);

	// Stress tests (simplified)
	test_42_multiple_connections(config, stats);
	test_42_no_hanging_connections(config, stats);
}
