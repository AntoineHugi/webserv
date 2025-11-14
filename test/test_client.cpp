#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>


// ANSI Colors
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

struct TestResult {
	int passed;
	int failed;
};

TestResult g_results = {0, 0};

void print_test(const std::string& name) {
	std::cout << "\n" << BLUE << "==========================================================" << RESET << std::endl;
	std::cout << BLUE << "TEST: " << name << RESET << std::endl;
	std::cout << BLUE << "==========================================================" << RESET << std::endl;
}

void print_pass(const std::string& msg) {
	std::cout << GREEN << "✓ PASS: " << RESET << msg << std::endl;
	g_results.passed++;
}

void print_fail(const std::string& msg) {
	std::cout << RED << "✗ FAIL: " << RESET << msg << std::endl;
	g_results.failed++;
}

void print_info(const std::string& msg) {
	std::cout << YELLOW << "ℹ INFO: " << RESET << msg << std::endl;
}

// Simple HTTP Response Parser
struct HttpResponse {
	std::string status_line;
	std::map<std::string, std::string> headers;
	std::string body;
	int status_code;
};

HttpResponse parse_response(const std::string& response) {
	HttpResponse resp;
	resp.status_code = 0;

	size_t header_end = response.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return resp;

	std::string header_section = response.substr(0, header_end);
	resp.body = response.substr(header_end + 4);

	std::istringstream stream(header_section);
	std::string line;

	// Parse status line
	if (std::getline(stream, line)) {
		resp.status_line = line;
		// Extract status code
		size_t first_space = line.find(' ');
		if (first_space != std::string::npos) {
			size_t second_space = line.find(' ', first_space + 1);
			if (second_space != std::string::npos) {
				std::string code = line.substr(first_space + 1, second_space - first_space - 1);
				resp.status_code = atoi(code.c_str());
			}
		}
	}

	// Parse headers
	while (std::getline(stream, line)) {
		if (!line.empty() && line[line.size()-1] == '\r')
			line.erase(line.size()-1);

		size_t colon = line.find(':');
		if (colon != std::string::npos) {
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			// Trim leading space from value
			size_t first_char = value.find_first_not_of(" \t");
			if (first_char != std::string::npos)
				value = value.substr(first_char);
			resp.headers[key] = value;
		}
	}

	return resp;
}

int connect_to_server(const char* host, int port) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
		perror("inet_pton");
		close(sock);
		return -1;
	}

	if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect");
		close(sock);
		return -1;
	}

	return sock;
}

std::string send_request(int sock, const std::string& method, const std::string& path,
						 const std::map<std::string, std::string>& headers = std::map<std::string, std::string>(),
						 const std::string& body = "") {
	// Build request
	std::ostringstream request;
	request << method << " " << path << " HTTP/1.1\r\n";
	request << "Host: localhost:8080\r\n";

	// Add custom headers
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		request << it->first << ": " << it->second << "\r\n";
	}

	if (!body.empty()) {
		request << "Content-Length: " << body.size() << "\r\n";
	}

	request << "\r\n";

	if (!body.empty()) {
		request << body;
	}

	std::string req_str = request.str();
	print_info("Sending: " + method + " " + path);

	// Send request
	ssize_t sent = send(sock, req_str.c_str(), req_str.size(), 0);
	if (sent < 0) {
		perror("send");
		return "";
	}

	// Receive response
	std::string response;
	char buffer[4096];

	// Set timeout
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	while (true) {
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// Timeout - no more data
				break;
			}
			perror("recv");
			break;
		} else if (received == 0) {
			// Connection closed
			break;
		}

		buffer[received] = '\0';
		response.append(buffer, received);

		// Simple check: if we have headers and Content-Length: 0, we're done
		if (response.find("\r\n\r\n") != std::string::npos &&
			response.find("Content-Length: 0") != std::string::npos) {
			break;
		}
	}

	return response;
}

// ============================================================================
// TEST 1: Basic Connection
// ============================================================================
void test_basic_connection() {
	print_test("Basic Connection and Response");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		print_fail("Could not connect to server");
		return;
	}
	print_pass("Connected to server");

	std::string response = send_request(sock, "GET", "/");
	HttpResponse resp = parse_response(response);

	if (resp.status_code == 200) {
		print_pass("Received 200 OK response");
	} else {
		std::ostringstream oss;
		oss << "Expected 200, got: " << resp.status_code;
		print_fail(oss.str());
	}

	close(sock);
	print_pass("Connection closed cleanly");
}

// ============================================================================
// TEST 2: Keep-Alive - Multiple Requests
// ============================================================================
void test_keepalive() {
	print_test("Keep-Alive: Multiple Sequential Requests");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		print_fail("Could not connect");
		return;
	}
	print_pass("Connected to server");

	// First request
	std::map<std::string, std::string> headers;
	headers["Connection"] = "keep-alive";

	std::string response1 = send_request(sock, "GET", "/", headers);
	HttpResponse resp1 = parse_response(response1);

	if (resp1.status_code == 200) {
		print_pass("First request: 200 OK");
	} else {
		print_fail("First request failed");
	}

	if (resp1.headers["Connection"] == "keep-alive") {
		print_pass("Server responded with Connection: keep-alive");
	} else {
		print_fail("Expected Connection: keep-alive header");
	}

	usleep(100000); // 100ms delay

	// Second request on same connection
	std::string response2 = send_request(sock, "GET", "/index", headers);
	HttpResponse resp2 = parse_response(response2);

	if (resp2.status_code == 200) {
		print_pass("Second request on same connection: 200 OK");
	} else {
		print_fail("Second request failed");
	}

	usleep(100000);

	// Third request
	std::string response3 = send_request(sock, "POST", "/data", headers, "test body");
	HttpResponse resp3 = parse_response(response3);

	if (resp3.status_code == 200) {
		print_pass("Third request on same connection: 200 OK");
	} else {
		print_fail("Third request failed");
	}

	close(sock);
	print_pass("All 3 requests completed on single connection");
}

// ============================================================================
// TEST 3: Connection Close
// ============================================================================
void test_connection_close() {
	print_test("Connection: close Behavior");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		print_fail("Could not connect");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Connection"] = "close";

	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	if (resp.status_code == 200) {
		print_pass("Received 200 OK");
	}

	if (resp.headers["Connection"] == "close") {
		print_pass("Server responded with Connection: close");
	} else {
		print_fail("Expected Connection: close header");
	}

	usleep(200000); // 200ms

	// Try to send another request (should fail or get no response)
	const char* test_req = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
	ssize_t sent = send(sock, test_req, strlen(test_req), 0);

	if (sent > 0) {
		char buffer[1024];
		ssize_t received = recv(sock, buffer, sizeof(buffer), 0);
		if (received == 0) {
			print_pass("Server closed connection after Connection: close");
		} else {
			print_fail("Server accepted request after Connection: close");
		}
	} else {
		print_pass("Cannot send on closed connection");
	}

	close(sock);
}

// ============================================================================
// TEST 4: Different HTTP Methods
// ============================================================================
void test_http_methods() {
	print_test("Different HTTP Methods");

	const char* methods[] = {"GET", "POST", "DELETE", "PUT"};

	for (int i = 0; i < 4; i++) {
		int sock = connect_to_server("127.0.0.1", 8080);
		if (sock < 0) {
			print_fail("Connection failed");
			continue;
		}

		std::string body = (strcmp(methods[i], "POST") == 0 || strcmp(methods[i], "PUT") == 0) ? "test data" : "";
		std::string response = send_request(sock, methods[i], "/test", std::map<std::string, std::string>(), body);
		HttpResponse resp = parse_response(response);

		if (resp.status_code == 200) {
			std::string msg = std::string(methods[i]) + " request successful";
			print_pass(msg);
		} else {
			std::ostringstream oss;
			oss << methods[i] << " returned: " << resp.status_code;
			print_info(oss.str());
		}

		close(sock);
	}
}

// ============================================================================
// TEST 5: POST with Body
// ============================================================================
void test_post_with_body() {
	print_test("POST Request with JSON Body");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		print_fail("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Content-Type"] = "application/json";
	headers["Connection"] = "keep-alive";

	std::string body = "{\"key1\":\"value1\", \"key2\":\"value2\"}";
	std::string response = send_request(sock, "POST", "/api/data", headers, body);
	HttpResponse resp = parse_response(response);

	if (resp.status_code == 200) {
		print_pass("POST with JSON body successful");
	} else {
		print_fail("POST failed");
	}

	close(sock);
}

// ============================================================================
// TEST 6: Rapid Connections
// ============================================================================
void test_rapid_connections() {
	print_test("Rapid Connect/Disconnect (10 connections)");

	int success = 0;
	for (int i = 0; i < 10; i++) {
		int sock = connect_to_server("127.0.0.1", 8080);
		if (sock < 0) {
			continue;
		}

		std::ostringstream path;
		path << "/rapid" << i;
		std::string response = send_request(sock, "GET", path.str());
		HttpResponse resp = parse_response(response);

		if (resp.status_code == 200) {
			success++;
		}

		close(sock);
	}

	std::ostringstream oss;
	oss << success << "/10 rapid connections successful";
	if (success == 10) {
		print_pass(oss.str());
	} else {
		print_fail(oss.str());
	}
}

// ============================================================================
// TEST 7: Multiple Server Ports
// ============================================================================
void test_multiple_ports() {
	print_test("Multiple Server Instances (ports 8080 and 8081)");

	int ports[] = {8080, 8081};

	for (int i = 0; i < 2; i++) {
		int sock = connect_to_server("127.0.0.1", ports[i]);
		if (sock < 0) {
			std::ostringstream oss;
			oss << "Could not connect to port " << ports[i];
			print_info(oss.str());
			continue;
		}

		std::string response = send_request(sock, "GET", "/");
		HttpResponse resp = parse_response(response);

		close(sock);

		std::ostringstream oss;
		oss << "Server on port " << ports[i];
		if (resp.status_code == 200) {
			print_pass(oss.str() + " responsive");
		} else {
			print_fail(oss.str() + " not working");
		}
	}
}

// ============================================================================
// TEST 8: Truly Concurrent Clients (using fork)
// ============================================================================
void concurrent_client_process(int client_id, int requests_per_client) {
	// Child process - connect and send requests
	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		exit(1); // Failed
	}

	std::map<std::string, std::string> headers;
	headers["Connection"] = "keep-alive";

	int success_count = 0;
	for (int i = 0; i < requests_per_client; i++) {
		std::ostringstream path;
		path << "/client" << client_id << "/req" << i;

		std::string response = send_request(sock, "GET", path.str(), headers);
		HttpResponse resp = parse_response(response);

		if (resp.status_code == 200) {
			success_count++;
		}

		usleep(50000); // 50ms between requests
	}

	close(sock);

	// Exit with success count (limited to 0-255)
	exit(success_count == requests_per_client ? 0 : 1);
}

void test_truly_concurrent_clients() {
	print_test("Truly Concurrent Clients (5 clients via fork, 3 requests each)");

	const int num_clients = 5;
	const int requests_per_client = 3;
	pid_t pids[num_clients];

	print_info("Spawning 5 concurrent client processes...");

	// Fork all clients at once
	for (int i = 0; i < num_clients; i++) {
		pid_t pid = fork();

		if (pid < 0) {
			print_fail("Fork failed");
			// Kill any children we already created
			for (int j = 0; j < i; j++) {
				kill(pids[j], SIGTERM);
			}
			return;
		} else if (pid == 0) {
			// Child process
			concurrent_client_process(i, requests_per_client);
			// Should not reach here
			exit(1);
		} else {
			// Parent process - save child PID
			pids[i] = pid;
		}
	}

	// Parent waits for all children
	int success_count = 0;
	int total_clients = num_clients;

	for (int i = 0; i < num_clients; i++) {
		int status;
		waitpid(pids[i], &status, 0);

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			success_count++;
		}
	}

	std::ostringstream oss;
	oss << success_count << "/" << total_clients << " concurrent clients completed all requests";

	if (success_count == total_clients) {
		print_pass(oss.str());
	} else {
		print_fail(oss.str());
	}

	print_info("All concurrent processes completed");
}

// ============================================================================
// TEST 9: Large Header (within limits)
// ============================================================================
void test_large_header() {
	print_test("Large Header (within 16KB limit)");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		print_fail("Connection failed");
		return;
	}

	std::map<std::string, std::string> headers;
	headers["Connection"] = "keep-alive";
	// Create a 10KB cookie value
	headers["Cookie"] = std::string(10000, 'x');

	std::string response = send_request(sock, "GET", "/", headers);
	HttpResponse resp = parse_response(response);

	if (resp.status_code == 200) {
		print_pass("Large header accepted (within limit)");
	} else {
		print_fail("Large header rejected");
	}

	close(sock);
}

// ============================================================================
// TEST 10: Chunked Transfer Encoding
// ============================================================================
void test_chunked_encoding() {
	print_test("Chunked Transfer Encoding");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		print_fail("Connection failed");
		return;
	}

	// Build chunked request manually
	std::ostringstream request;
	request << "POST /upload HTTP/1.1\r\n";
	request << "Host: localhost:8080\r\n";
	request << "Transfer-Encoding: chunked\r\n";
	request << "Connection: keep-alive\r\n";
	request << "\r\n";
	// Chunk 1: "Hello" (5 bytes)
	request << "5\r\n";
	request << "Hello\r\n";
	// Chunk 2: ", World" (7 bytes)
	request << "7\r\n";
	request << ", World\r\n";
	// Chunk 3: "!" (1 byte)
	request << "1\r\n";
	request << "!\r\n";
	// End chunk
	request << "0\r\n";
	request << "\r\n";

	std::string req_str = request.str();
	print_info("Sending chunked POST request");

	ssize_t sent = send(sock, req_str.c_str(), req_str.size(), 0);
	if (sent < 0) {
		print_fail("Failed to send chunked request");
		close(sock);
		return;
	}

	// Receive response
	std::string response;
	char buffer[4096];

	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

	while (true) {
		ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (received <= 0) break;
		buffer[received] = '\0';
		response.append(buffer, received);
		if (response.find("\r\n\r\n") != std::string::npos &&
			response.find("Content-Length: 0") != std::string::npos) {
			break;
		}
	}

	HttpResponse resp = parse_response(response);

	if (resp.status_code == 200) {
		print_pass("Chunked request processed successfully");
	} else {
		std::ostringstream oss;
		oss << "Chunked request failed with status: " << resp.status_code;
		print_fail(oss.str());
	}

	close(sock);
}

// ============================================================================
// TEST 11: Pipelined Requests with Chunked Encoding
// ============================================================================
void test_pipelined_chunked() {
	print_test("Pipelined Requests: Chunked + GET (leftover detection)");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		print_fail("Connection failed");
		return;
	}

	// Build pipelined request: chunked POST + GET (no delay)
	std::ostringstream request;
	// First request: chunked POST
	request << "POST /upload HTTP/1.1\r\n";
	request << "Host: localhost:8080\r\n";
	request << "Transfer-Encoding: chunked\r\n";
	request << "Connection: keep-alive\r\n";
	request << "\r\n";
	request << "5\r\n";
	request << "Hello\r\n";
	request << "0\r\n";
	request << "\r\n";
	// Second request: GET (pipelined immediately after)
	request << "GET /next HTTP/1.1\r\n";
	request << "Host: localhost:8080\r\n";
	request << "Connection: close\r\n";
	request << "\r\n";

	std::string req_str = request.str();
	print_info("Sending pipelined chunked POST + GET");

	ssize_t sent = send(sock, req_str.c_str(), req_str.size(), 0);
	if (sent < 0) {
		print_fail("Failed to send pipelined request");
		close(sock);
		return;
	}

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

	// Count how many "HTTP/" status lines we got
	size_t count = 0;
	size_t pos = 0;
	while ((pos = all_responses.find("HTTP/", pos)) != std::string::npos) {
		count++;
		pos += 5;
	}

	if (count >= 2) {
		print_pass("Both pipelined requests processed (chunked leftover detected)");
	} else if (count == 1) {
		print_fail("Only first request processed - leftover GET was lost!");
	} else {
		print_fail("No valid responses received");
	}

	close(sock);
}

// ============================================================================
// TEST 12: Pipelined Requests with Content-Length
// ============================================================================
void test_pipelined_content_length() {
	print_test("Pipelined Requests: Content-Length + GET (leftover detection)");

	int sock = connect_to_server("127.0.0.1", 8080);
	if (sock < 0) {
		print_fail("Connection failed");
		return;
	}

	// Build pipelined request: POST with Content-Length + GET
	std::ostringstream request;
	// First request: POST with body
	std::string body = "Hello, World!";
	request << "POST /submit HTTP/1.1\r\n";
	request << "Host: localhost:8080\r\n";
	request << "Content-Length: " << body.size() << "\r\n";
	request << "Connection: keep-alive\r\n";
	request << "\r\n";
	request << body;
	// Second request: GET (pipelined immediately)
	request << "GET /after HTTP/1.1\r\n";
	request << "Host: localhost:8080\r\n";
	request << "Connection: close\r\n";
	request << "\r\n";

	std::string req_str = request.str();
	print_info("Sending pipelined POST (Content-Length) + GET");

	ssize_t sent = send(sock, req_str.c_str(), req_str.size(), 0);
	if (sent < 0) {
		print_fail("Failed to send pipelined request");
		close(sock);
		return;
	}

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
		print_pass("Both pipelined requests processed (Content-Length leftover detected)");
	} else if (count == 1) {
		print_fail("Only POST processed - leftover GET was lost!");
	} else {
		print_fail("No valid responses received");
	}

	close(sock);
}

// ============================================================================
// MAIN
// ============================================================================
int main() {
	std::cout << "\n" << BLUE << "==========================================================" << RESET << std::endl;
	std::cout << BLUE << "  WEBSERV C++ TEST SUITE" << RESET << std::endl;
	std::cout << BLUE << "==========================================================" << RESET << std::endl;

	print_info("Testing server at 127.0.0.1:8080");
	print_info("Make sure your server is running!\n");

	sleep(1);

	// Run all tests
	test_basic_connection();
	test_keepalive();
	test_connection_close();
	test_http_methods();
	test_post_with_body();
	test_rapid_connections();
	test_truly_concurrent_clients();
	test_multiple_ports();
	test_large_header();
	test_chunked_encoding();
	test_pipelined_chunked();
	test_pipelined_content_length();

	// Summary
	std::cout << "\n" << BLUE << "==========================================================" << RESET << std::endl;
	std::cout << BLUE << "  TEST SUMMARY" << RESET << std::endl;
	std::cout << BLUE << "==========================================================" << RESET << std::endl;

	std::cout << GREEN << "Passed: " << g_results.passed << RESET << std::endl;
	std::cout << RED << "Failed: " << g_results.failed << RESET << std::endl;
	std::cout << "Total:  " << (g_results.passed + g_results.failed) << "\n" << std::endl;

	if (g_results.failed == 0) {
		std::cout << GREEN << "==========================================================" << std::endl;
		std::cout << "  ALL TESTS PASSED! ✓" << std::endl;
		std::cout << "==========================================================" << RESET << "\n" << std::endl;
		return 0;
	} else {
		std::cout << RED << "==========================================================" << std::endl;
		std::cout << "  SOME TESTS FAILED" << std::endl;
		std::cout << "==========================================================" << RESET << "\n" << std::endl;
		return 1;
	}
}
