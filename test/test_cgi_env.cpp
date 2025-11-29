#include "test_framework.hpp"
#include <sys/socket.h>

// ============================================================================
// CGI ENVIRONMENT VARIABLE TESTS
// ============================================================================

void test_cgi_env_request_method_get(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: REQUEST_METHOD (GET)");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/env_test.py", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check REQUEST_METHOD is set to GET
	if (resp.body.find("\"REQUEST_METHOD\": \"GET\"") != std::string::npos) {
		stats.add_pass();
		print_pass("REQUEST_METHOD=GET correctly set");
	} else {
		stats.add_fail();
		print_fail("REQUEST_METHOD not set or incorrect");
		if (config.verbose) {
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_request_method_post(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: REQUEST_METHOD (POST)");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/plain";

	std::string body = "test data";
	std::string response = send_request(sock, "POST", "/cgi/env_test.py", headers, body);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check REQUEST_METHOD is set to POST
	if (resp.body.find("\"REQUEST_METHOD\": \"POST\"") != std::string::npos) {
		stats.add_pass();
		print_pass("REQUEST_METHOD=POST correctly set");
	} else {
		stats.add_fail();
		print_fail("REQUEST_METHOD not set or incorrect");
		if (config.verbose) {
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_query_string(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: QUERY_STRING");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/env_test.py?name=value&foo=bar", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check QUERY_STRING contains the parameters
	if (resp.body.find("\"QUERY_STRING\": \"name=value&foo=bar\"") != std::string::npos ||
	    resp.body.find("\"QUERY_STRING\": \"name=value\\u0026foo=bar\"") != std::string::npos) {
		stats.add_pass();
		print_pass("QUERY_STRING correctly parsed");
	} else {
		stats.add_fail();
		print_fail("QUERY_STRING not set or incorrect");
		if (config.verbose) {
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_content_type(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: CONTENT_TYPE");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "text/x-python";

	std::string body = "{\"test\": \"data\"}";
	std::string response = send_request(sock, "POST", "/cgi/env_test.py", headers, body);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check CONTENT_TYPE is set from request header
	if (resp.body.find("\"CONTENT_TYPE\": \"text/x-python\"") != std::string::npos) {
		stats.add_pass();
		print_pass("CONTENT_TYPE correctly set");
	} else {
		stats.add_fail();
		print_fail("CONTENT_TYPE not set or incorrect");
		if (config.verbose) {
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_content_length(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: CONTENT_LENGTH");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string body = "Hello World";  // 11 bytes
	std::string response = send_request(sock, "POST", "/cgi/env_test.py", headers, body);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check CONTENT_LENGTH matches body size
	if (resp.body.find("\"CONTENT_LENGTH\": \"11\"") != std::string::npos) {
		stats.add_pass();
		print_pass("CONTENT_LENGTH correctly set");
	} else {
		stats.add_fail();
		print_fail("CONTENT_LENGTH not set or incorrect");
		if (config.verbose) {
			std::cout << "Expected CONTENT_LENGTH: 11" << std::endl;
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_server_protocol(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: SERVER_PROTOCOL");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/env_test.py", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check SERVER_PROTOCOL is HTTP/1.1 or HTTP/1.0
	bool has_protocol = (resp.body.find("\"SERVER_PROTOCOL\": \"HTTP/1.1\"") != std::string::npos ||
	                     resp.body.find("\"SERVER_PROTOCOL\": \"HTTP/1.0\"") != std::string::npos);

	if (has_protocol) {
		stats.add_pass();
		print_pass("SERVER_PROTOCOL correctly set");
	} else {
		stats.add_fail();
		print_fail("SERVER_PROTOCOL not set or incorrect");
		if (config.verbose) {
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_gateway_interface(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: GATEWAY_INTERFACE");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/env_test.py", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check GATEWAY_INTERFACE is set to CGI/1.1
	if (resp.body.find("\"GATEWAY_INTERFACE\": \"CGI/1.1\"") != std::string::npos) {
		stats.add_pass();
		print_pass("GATEWAY_INTERFACE correctly set");
	} else {
		stats.add_fail();
		print_fail("GATEWAY_INTERFACE not set or incorrect");
		if (config.verbose) {
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_script_name(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: SCRIPT_NAME");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/env_test.py", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check SCRIPT_NAME contains the script path
	if (resp.body.find("\"SCRIPT_NAME\"") != std::string::npos &&
	    resp.body.find("/cgi/env_test.py") != std::string::npos) {
		stats.add_pass();
		print_pass("SCRIPT_NAME correctly set");
	} else {
		stats.add_fail();
		print_fail("SCRIPT_NAME not set or incorrect");
		if (config.verbose) {
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_server_port(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: SERVER_PORT");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/env_test.py", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// Check SERVER_PORT is set (should match config.server_port)
	std::ostringstream ss;
	ss << config.server_port;
	std::string port_str = "\"SERVER_PORT\": \"" + ss.str() + "\"";

	if (resp.body.find(port_str) != std::string::npos) {
		stats.add_pass();
		print_pass("SERVER_PORT correctly set");
	} else {
		stats.add_fail();
		print_fail("SERVER_PORT not set or incorrect");
		if (config.verbose) {
			std::cout << "Expected: " << port_str << std::endl;
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

void test_cgi_env_empty_query_string(const TestConfig& config, TestStats& stats)
{
	print_test("CGI ENV: Empty QUERY_STRING");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/cgi/env_test.py", headers);
	HttpResponse resp = parse_response(response);

	if (!assert_status(resp, 200, stats)) {
		close(sock);
		return;
	}

	// QUERY_STRING should be empty when no query parameters
	if (resp.body.find("\"QUERY_STRING\": \"\"") != std::string::npos) {
		stats.add_pass();
		print_pass("Empty QUERY_STRING correctly handled");
	} else {
		stats.add_fail();
		print_fail("QUERY_STRING should be empty");
		if (config.verbose) {
			std::cout << "Body: " << resp.body << std::endl;
		}
	}

	close(sock);
}

// ============================================================================
// CGI ENV TEST RUNNER
// ============================================================================

void run_cgi_env_tests(const TestConfig& config, TestStats& stats)
{
	print_category("CGI ENVIRONMENT VARIABLES");

	test_cgi_env_request_method_get(config, stats);
	test_cgi_env_request_method_post(config, stats);
	// test_cgi_env_query_string(config, stats); //feature not implemented
	test_cgi_env_content_type(config, stats);
	test_cgi_env_content_length(config, stats);
	test_cgi_env_server_protocol(config, stats);
	test_cgi_env_gateway_interface(config, stats);
	test_cgi_env_script_name(config, stats);
	test_cgi_env_server_port(config, stats);
	test_cgi_env_empty_query_string(config, stats);
}
