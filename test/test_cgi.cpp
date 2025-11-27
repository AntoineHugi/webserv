#include "test_framework.hpp"
#include <sys/socket.h>
#include <cerrno>
#include <ctime>

// ============================================================================
// CGI TESTS
// ============================================================================

void test_cgi_get_simple(const TestConfig& config, TestStats& stats)
{
	print_test("CGI: Simple GET Request");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/hello.py", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check that we got CGI output
	if (resp.body.find("Hello from CGI") != std::string::npos) {
		stats.add_pass();
		print_pass("CGI response correct");
	} else {
		stats.add_fail();
		print_fail("Expected 'Hello from CGI' in response body");
		if (config.verbose) {
			std::cout << "Got: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_post_echo(const TestConfig& config, TestStats& stats)
{
	print_test("CGI: POST Echo (Reverse)");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/plain";

	std::string body = "Hello World";
	std::string response = send_request(sock, "POST", "/cgi/echo.py", headers, body);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Echo script reverses the input
	std::string expected = "dlroW olleH";
	if (resp.body.find(expected) != std::string::npos) {
		stats.add_pass();
		print_pass("CGI echoed and reversed POST data");
	} else {
		stats.add_fail();
		print_fail("Expected reversed echo");
		if (config.verbose) {
			std::cout << "Expected: " << expected << std::endl;
			std::cout << "Got: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_post_uppercase(const TestConfig& config, TestStats& stats)
{
	print_test("CGI: POST Uppercase Conversion");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/plain";

	std::string body = "convert this to uppercase";
	std::string response = send_request(sock, "POST", "/cgi/uppercase.py", headers, body);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check for uppercase conversion
	if (resp.body.find("CONVERT THIS TO UPPERCASE") != std::string::npos) {
		stats.add_pass();
		print_pass("CGI converted to uppercase");
	} else {
		stats.add_fail();
		print_fail("Expected uppercase conversion");
		if (config.verbose) {
			std::cout << "Got: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_error_handling(const TestConfig& config, TestStats& stats)
{
	print_test("CGI: Error Handling (Exit Code 1)");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/error.py", headers);
	HttpResponse resp = parse_response(response);

	// CGI script exits with error code 1
	// Server should return 500 Internal Server Error
	if (assert_status(resp, 500, stats)) {
		print_pass("Server returned 500 for failed CGI");
	}

	close(sock);
}

void test_cgi_large_output(const TestConfig& config, TestStats& stats)
{
	print_test("CGI: Large Output (10KB)");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/large.py", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check that we got substantial output (at least 8KB)
	if (resp.body.size() > 8192) {
		stats.add_pass();
		print_pass("Received large CGI output");
		if (config.verbose) {
			std::cout << "Received " << resp.body.size() << " bytes" << std::endl;
		}
	} else {
		stats.add_fail();
		print_fail("Large output too small");
		if (config.verbose) {
			std::cout << "Expected >8192 bytes, got " << resp.body.size() << std::endl;
		}
	}

	close(sock);
}

void test_cgi_slow_response(const TestConfig& config, TestStats& stats)
{
	print_test("CGI: Slow Response (2 seconds)");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;

	time_t start = time(NULL);
	std::string response = send_request(sock, "GET", "/cgi/slow.py", headers);
	time_t end = time(NULL);

	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Should take at least 2 seconds
	int elapsed = end - start;
	if (elapsed >= 2 && resp.body.find("Slow response") != std::string::npos) {
		stats.add_pass();
		print_pass("CGI slow response handled correctly");
		if (config.verbose) {
			std::cout << "Took " << elapsed << " seconds" << std::endl;
		}
	} else {
		stats.add_fail();
		print_fail("CGI slow response issue");
		if (config.verbose) {
			std::cout << "Elapsed: " << elapsed << " seconds" << std::endl;
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_json_output(const TestConfig& config, TestStats& stats)
{
	print_test("CGI: JSON Output");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "application/json";

	std::string body = "test data";
	std::string response = send_request(sock, "POST", "/cgi/json_test.py", headers, body);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check Content-Type is JSON
	bool is_json = (resp.headers["Content-Type"].find("application/json") != std::string::npos);

	// Check body contains JSON structure
	bool has_json = (resp.body.find("\"received\"") != std::string::npos &&
	                 resp.body.find("\"status\"") != std::string::npos &&
	                 resp.body.find("test data") != std::string::npos);

	if (is_json && has_json) {
		stats.add_pass();
		print_pass("CGI JSON response correct");
	} else {
		stats.add_fail();
		print_fail("CGI JSON response incorrect");
		if (config.verbose) {
			std::cout << "Content-Type: " << resp.headers["Content-Type"] << std::endl;
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_keep_alive(const TestConfig& config, TestStats& stats)
{
	print_test("CGI: Keep-Alive After CGI Request");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	// First request to CGI
	std::map<std::string, std::string> headers;
	headers["Connection"] = "keep-alive";
	std::string response1 = send_request(sock, "GET", "/cgi/hello.py", headers);
	HttpResponse resp1 = parse_response(response1);

	if (!assert_status(resp1, 200, stats)) {
		close(sock);
		return;
	}

	// Second request on same socket
	std::string response2 = send_request(sock, "GET", "/cgi/hello.py", headers);
	HttpResponse resp2 = parse_response(response2);

	if (assert_status(resp2, 200, stats)) {
		print_pass("Keep-alive works with CGI");
	}

	close(sock);
}

// ============================================================================
// CGI TEST RUNNER
// ============================================================================

void run_cgi_tests(const TestConfig& config, TestStats& stats)
{
	print_category("CGI EXECUTION");

	test_cgi_get_simple(config, stats);
	test_cgi_post_echo(config, stats);
	test_cgi_post_uppercase(config, stats);
	test_cgi_error_handling(config, stats);
	test_cgi_large_output(config, stats);
	test_cgi_slow_response(config, stats);
	test_cgi_json_output(config, stats);
	test_cgi_keep_alive(config, stats);
}
