#include "test_framework.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <cerrno>

// C++98 compatible int to string
static std::string int_to_string(int value) {
	std::ostringstream ss;
	ss << value;
	return ss.str();
}

// ============================================================================
// CATEGORY: PIPELINED REQUESTS
// ============================================================================

void test_pipeline_chunked_get(const TestConfig& config, TestStats& stats) {
	print_test("Pipeline: Chunked POST + GET");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	// Send both requests in one TCP send
	std::string request;
	request += "POST /upload HTTP/1.1\r\n";
	request += "Host: localhost:8080\r\n";
	request += "Transfer-Encoding: chunked\r\n";
	request += "Connection: keep-alive\r\n";
	request += "\r\n";
	request += "5\r\n";
	request += "Hello\r\n";
	request += "0\r\n";
	request += "\r\n";
	// Second request immediately after
	request += "GET /next HTTP/1.1\r\n";
	request += "Host: localhost:8080\r\n";
	request += "Connection: close\r\n";
	request += "\r\n";

	print_info("Sending pipelined chunked POST + GET");
	send(sock, request.c_str(), request.size(), 0);

	// Receive both responses
	std::string all_responses;
	char buffer[4096];

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	while (true) {
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0) break;
		buffer[received] = '\0';
		all_responses.append(buffer, received);
	}

	// Count HTTP responses
	size_t count = 0;
	size_t pos = 0;
	while ((pos = all_responses.find("HTTP/", pos)) != std::string::npos) {
		count++;
		pos += 5;
	}

	if (count >= 2) {
		stats.add_pass();
		print_pass("Both pipelined requests processed");
	} else if (count == 1) {
		stats.add_fail();
		print_fail("Only first request processed - leftover lost");
	} else {
		stats.add_fail();
		print_fail("No valid responses received");
	}

	close(sock);
}

void test_pipeline_content_length_get(const TestConfig& config, TestStats& stats) {
	print_test("Pipeline: Content-Length POST + GET");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string body = "Hello, World!";
	std::string request;
	request += "POST /submit HTTP/1.1\r\n";
	request += "Host: localhost:8080\r\n";
	request += "Content-Length: 13\r\n";
	request += "Connection: keep-alive\r\n";
	request += "\r\n";
	request += body;
	// Second request
	request += "GET / HTTP/1.1\r\n";
	request += "Host: localhost:8080\r\n";
	request += "Connection: close\r\n";
	request += "\r\n";

	print_info("Sending pipelined Content-Length POST + GET");
	send(sock, request.c_str(), request.size(), 0);

	std::string all_responses;
	char buffer[4096];

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	while (true) {
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0) break;
		buffer[received] = '\0';
		all_responses.append(buffer, received);
	}

	size_t count = 0;
	size_t pos = 0;
	while ((pos = all_responses.find("HTTP/", pos)) != std::string::npos) {
		count++;
		pos += 5;
	}

	if (count >= 2) {
		stats.add_pass();
		print_pass("Both pipelined requests processed");
	} else {
		stats.add_fail();
		print_fail("Pipeline failed - only " + int_to_string(count) + " response(s)");
	}

	close(sock);
}

void test_pipeline_three_requests(const TestConfig& config, TestStats& stats) {
	print_test("Pipeline: Three Sequential GETs");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	std::string request;
	request += "GET /1 HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n";
	request += "GET /2 HTTP/1.1\r\nHost: localhost:8080\r\nConnection: keep-alive\r\n\r\n";
	request += "GET /3 HTTP/1.1\r\nHost: localhost:8080\r\nConnection: close\r\n\r\n";

	print_info("Sending 3 pipelined GETs");
	send(sock, request.c_str(), request.size(), 0);

	std::string all_responses;
	char buffer[4096];

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	while (true) {
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0) break;
		buffer[received] = '\0';
		all_responses.append(buffer, received);
	}

	size_t count = 0;
	size_t pos = 0;
	while ((pos = all_responses.find("HTTP/", pos)) != std::string::npos) {
		count++;
		pos += 5;
	}

	if (count >= 3) {
		stats.add_pass();
		print_pass("All 3 pipelined requests processed");
	} else {
		stats.add_fail();
		print_fail("Only " + int_to_string(count) + "/3 requests processed");
	}

	close(sock);
}

// ============================================================================
// CATEGORY RUNNER
// ============================================================================

void run_pipelining_tests(const TestConfig& config, TestStats& stats) {
	print_category("PIPELINED REQUESTS (HTTP/1.1)");

	test_pipeline_chunked_get(config, stats);
	test_pipeline_content_length_get(config, stats);
	test_pipeline_three_requests(config, stats);
}
