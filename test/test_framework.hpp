#ifndef TEST_FRAMEWORK_HPP
#define TEST_FRAMEWORK_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>

// ============================================================================
// TEST CONFIGURATION
// ============================================================================

struct TestConfig {
	std::string server_host;
	int server_port;
	std::map<std::string, bool> enabled_categories;
	bool verbose;
	bool stop_on_fail;
};

// ============================================================================
// TEST RESULTS TRACKING
// ============================================================================

struct TestStats {
	int total;
	int passed;
	int failed;
	int skipped;

	TestStats() : total(0), passed(0), failed(0), skipped(0) {}

	void add_pass() { total++; passed++; }
	void add_fail() { total++; failed++; }
	void add_skip() { total++; skipped++; }

	double pass_rate() const {
		if (total == 0) return 0.0;
		return (100.0 * passed) / total;
	}
};

// ============================================================================
// HTTP RESPONSE PARSING
// ============================================================================

struct HttpResponse {
	std::string status_line;
	std::map<std::string, std::string> headers;
	std::string body;
	int status_code;
	std::string raw_response;

	HttpResponse() : status_code(0) {}
};

// ============================================================================
// TEST OUTPUT UTILITIES
// ============================================================================

namespace Color {
	extern const char* const RESET;
	extern const char* const RED;
	extern const char* const GREEN;
	extern const char* const YELLOW;
	extern const char* const BLUE;
	extern const char* const MAGENTA;
	extern const char* const CYAN;
	extern const char* const BOLD;
}

void print_header(const std::string& text);
void print_category(const std::string& text);
void print_test(const std::string& text);
void print_pass(const std::string& text);
void print_fail(const std::string& text);
void print_skip(const std::string& text);
void print_info(const std::string& text);
void print_debug(const std::string& text);

// ============================================================================
// NETWORK UTILITIES
// ============================================================================

int connect_to_server(const char* host, int port);
std::string send_request(int sock, const std::string& method, const std::string& path,
						 const std::map<std::string, std::string>& headers = std::map<std::string, std::string>(),
						 const std::string& body = "");
HttpResponse parse_response(const std::string& response);

// ============================================================================
// TEST ASSERTION HELPERS
// ============================================================================

bool assert_status(const HttpResponse& resp, int expected_code, TestStats& stats);
bool assert_header(const HttpResponse& resp, const std::string& header,
				   const std::string& expected_value, TestStats& stats);
bool assert_header_exists(const HttpResponse& resp, const std::string& header, TestStats& stats);
bool assert_body_contains(const HttpResponse& resp, const std::string& text, TestStats& stats);
bool assert_connection_closed(int sock, TestStats& stats);

#endif
