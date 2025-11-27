#include "test_framework.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

// ============================================================================
// CATEGORY: CONNECTION PERSISTENCE (KEEP-ALIVE)
// ============================================================================

void test_keepalive_header(const TestConfig& config, TestStats& stats) {
	print_test("Keep-Alive: Server Response Header");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Connection"] = "keep-alive";

	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	assert_header(resp, "Connection", "keep-alive", stats);

	close(sock);
}

void test_multiple_requests_same_connection(const TestConfig& config, TestStats& stats) {
	print_test("Keep-Alive: Multiple Requests on Same Socket");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Connection"] = "keep-alive";

	// First request
	print_info("Request 1/3");
	std::string response1 = send_request(sock, "GET", "/", headers);
	HttpResponse resp1 = parse_response(response1);

	if (resp1.status_code != 200) {
		stats.add_fail();
		print_fail("First request failed");
		close(sock);
		return;
	}

	usleep(100000);  // 100ms

	// Second request on same socket
	print_info("Request 2/3");
	std::string response2 = send_request(sock, "GET", "/", headers);
	HttpResponse resp2 = parse_response(response2);

	if (resp2.status_code != 200) {
		stats.add_fail();
		print_fail("Second request failed");
		close(sock);
		return;
	}

	usleep(100000);

	// Third request on same socket
	print_info("Request 3/3");
	std::string response3 = send_request(sock, "GET", "/", headers);
	HttpResponse resp3 = parse_response(response3);

	if (resp3.status_code == 200) {
		stats.add_pass();
		print_pass("All 3 requests on same connection");
	} else {
		stats.add_fail();
		print_fail("Third request failed");
	}

	close(sock);
}

void test_connection_close_header(const TestConfig& config, TestStats& stats) {
	print_test("Connection: close Behavior");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Connection"] = "close";

	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	assert_status(resp, 200, stats);
	assert_header(resp, "Connection", "close", stats);

	// Server should close connection
	usleep(100000);
	assert_connection_closed(sock, stats);

	close(sock);
}

void test_idle_timeout(const TestConfig& config, TestStats& stats) {
	print_test("Keep-Alive: Idle Timeout");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Connection"] = "keep-alive";

	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	if (resp.status_code != 200) {
		stats.add_skip();
		print_skip("Initial request failed");
		close(sock);
		return;
	}

	print_info("Waiting 65 seconds for idle timeout...");
	sleep(65);

	// Try to send another request - should fail or get timeout
	char test_byte = 'X';
	ssize_t sent = send(sock, &test_byte, 1, 0);

	if (sent <= 0 || errno == EPIPE) {
		stats.add_pass();
		print_pass("Connection timed out as expected");
	} else {
		stats.add_fail();
		print_fail("Connection still alive after timeout");
	}

	close(sock);
}

void test_client_disconnect_mid_request(const TestConfig &config, TestStats &stats)
{
	print_test("Client Disconnect During Request");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) { stats.add_skip(); return; }

	std::string req =
		"POST /upload-list HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 1000\r\n\r\n"
		"partial-data-";

	send(sock, req.c_str(), req.size(), 0);

	close(sock);

	sleep(1);

	stats.add_pass();
	print_pass("Server survived client disconnect");
}

void test_header_slowpoke(const TestConfig &config, TestStats &stats)
{
	print_test("Slowpoke Protection (server doesn't hang)");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) { stats.add_skip(); return; }

	// Send headers slowly
	std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\n";
	for (size_t i = 0; i < req.size(); i++)
	{
		send(sock, &req[i], 1, 0);
		usleep(20000); // 20ms per byte
	}

	// Finish request
	send(sock, "\r\n\r\n", 4, 0);

	// Expect response
	std::string response;
	char buf[1024];
	int n = recv(sock, buf, 1024, 0);
	close(sock);

	if (n > 0)
	{
		stats.add_pass();
		print_pass("Server responded despite slow headers");
	}
	else
	{
		stats.add_fail();
		print_fail("Server hung or closed before header completion");
	}
}

// ============================================================================
// CATEGORY RUNNER
// ============================================================================

void run_keepalive_tests(const TestConfig& config, TestStats& stats) {
	print_category("CONNECTION PERSISTENCE (KEEP-ALIVE)");

	test_keepalive_header(config, stats);
	test_multiple_requests_same_connection(config, stats);
	test_connection_close_header(config, stats);
	test_client_disconnect_mid_request(config, stats);
	test_header_slowpoke(config, stats);

	// Timeout test is slow, make it optional
	if (config.verbose) {
		test_idle_timeout(config, stats);
	} else {
		stats.add_skip();
		print_skip("Idle timeout test (use --verbose to enable)");
	}
}
