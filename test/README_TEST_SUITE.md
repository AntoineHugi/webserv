# Webserv Test Suite

A comprehensive, modular test suite for validating HTTP/1.1 server compliance.

## Features

✅ **Modular Categories** - Test specific aspects independently
✅ **Config-Driven** - Adapts to server configuration
✅ **Flexible Execution** - Run all, specific categories, or individual tests
✅ **Small Buffer Testing** - Uses 100-byte chunks to expose edge cases
✅ **Clear Reporting** - Color-coded pass/fail/skip with statistics
✅ **CI/CD Ready** - Returns proper exit codes

## Quick Start

### 1. Build the Test Suite

```bash
cd test
make -f Makefile.tests
```

### 2. Start Your Server

```bash
cd ..
./webserv default.conf
```

### 3. Run Tests

```bash
cd test
./test_suite                # Run all tests
./test_suite --only basic   # Run only basic tests
./test_suite --help         # Show all options
```

## Test Categories

### **basic** - Basic Connectivity & Requests
- Server connection
- Simple GET requests
- 404 Not Found
- HTTP version validation
- Required headers

### **keepalive** - Connection Persistence
- Keep-Alive header handling
- Multiple sequential requests
- Connection: close behavior
- Idle timeout (optional, slow)

### **chunked** - Chunked Transfer Encoding
- Simple chunked POST
- Multiple chunks
- Chunked with keep-alive
- Leftover data handling

### **pipeline** - Pipelined Requests
- Chunked POST + GET pipeline
- Content-Length POST + GET pipeline
- Three sequential GETs
- Leftover detection

## Usage Examples

### Run Specific Categories

```bash
# Run only basic connectivity tests
./test_suite --only basic

# Run keep-alive and chunked tests
./test_suite --skip pipeline --skip basic

# Run with verbose output (includes slow tests)
./test_suite --verbose
```

### Test During Development

```bash
# Stop on first failure
./test_suite --stop-on-fail

# Test specific functionality
./test_suite --only chunked
```

### Automated Testing

```bash
# Full test with auto server start/stop
make -f Makefile.tests test-full

# Or use individual targets
make -f Makefile.tests test-basic
make -f Makefile.tests test-keepalive
make -f Makefile.tests test-chunked
make -f Makefile.tests test-pipeline
```

## Understanding Test Results

### Color Coding

- 🟢 **Green (✓ PASS)** - Test passed
- 🔴 **Red (✗ FAIL)** - Test failed
- 🟡 **Yellow (○ SKIP)** - Test skipped (feature not implemented or connection issue)
- 🔵 **Blue (ℹ INFO)** - Informational message

### Exit Codes

- `0` - All tests passed
- `1` - One or more tests failed

### Statistics

At the end of each run, you'll see:
```
========================================
  TEST SUMMARY
========================================
Total:   45
Passed:  42
Failed:  1
Skipped: 2

Pass rate: 93.3%
```

## Test Configuration

### Command-Line Options

```
--host <host>        Server hostname (default: 127.0.0.1)
--port <port>        Server port (default: 8080)
--verbose            Enable verbose output (includes slow tests)
--stop-on-fail       Stop testing on first failure
--only <category>    Run only specific category
--skip <category>    Skip specific category
--list               List available test categories
--help               Show help message
```

### Examples

```bash
# Test on different port
./test_suite --port 8081

# Test remote server
./test_suite --host 192.168.1.100 --port 80

# Run only basic tests with verbose output
./test_suite --only basic --verbose

# Skip slow tests
./test_suite --skip keepalive
```

## Adding New Tests

### 1. Create a Test Category File

```cpp
// test_newcategory.cpp
#include "test_framework.hpp"

void test_my_feature(const TestConfig& config, TestStats& stats) {
	print_test("My Feature Test");

	int sock = connect_to_server(config.server_host.c_str(), config.server_port);
	if (sock < 0) {
		stats.add_skip();
		print_skip("Connection failed");
		return;
	}

	// Your test logic here
	std::map<std::string, std::string> headers;
	std::string response = send_request(sock, "GET", "/test", headers);
	HttpResponse resp = parse_response(response);

	assert_status(resp, 200, stats);

	close(sock);
}

void run_newcategory_tests(const TestConfig& config, TestStats& stats) {
	print_category("NEW CATEGORY");
	test_my_feature(config, stats);
}
```

### 2. Add to test_suite.cpp

```cpp
// Forward declaration
void run_newcategory_tests(const TestConfig& config, TestStats& stats);

// In main()
if (config.enabled_categories["newcategory"]) {
	run_newcategory_tests(config, stats);
}
```

### 3. Update Makefile

```makefile
CATEGORY_SRCS := test_basic.cpp test_keepalive.cpp test_chunked.cpp \
                 test_pipelining.cpp test_newcategory.cpp
```

## Test Framework API

### Assertion Functions

```cpp
bool assert_status(const HttpResponse& resp, int expected_code, TestStats& stats);
bool assert_header(const HttpResponse& resp, const std::string& header,
				   const std::string& expected_value, TestStats& stats);
bool assert_header_exists(const HttpResponse& resp, const std::string& header,
						  TestStats& stats);
bool assert_body_contains(const HttpResponse& resp, const std::string& text,
						  TestStats& stats);
bool assert_connection_closed(int sock, TestStats& stats);
```

### Output Functions

```cpp
void print_test(const std::string& text);    // Test name
void print_pass(const std::string& text);    // Success message
void print_fail(const std::string& text);    // Failure message
void print_skip(const std::string& text);    // Skip message
void print_info(const std::string& text);    // Info message
void print_debug(const std::string& text);   // Debug message
```

### Network Functions

```cpp
int connect_to_server(const char* host, int port);
std::string send_request(int sock, const std::string& method,
						 const std::string& path,
						 const std::map<std::string, std::string>& headers,
						 const std::string& body);
HttpResponse parse_response(const std::string& response);
```

## Troubleshooting

### Tests Fail to Connect

```bash
# Make sure your server is running
./webserv default.conf

# Check if port is correct
./test_suite --port 8080
```

### Many Tests Failing

```bash
# Run verbose to see detailed output
./test_suite --verbose

# Run one category at a time
./test_suite --only basic
```

### Tests Hang

- Check for deadlocks in your server
- Make sure keep-alive timeout is reasonable (< 60s)
- Use `--skip keepalive` to skip timeout tests

## Best Practices

1. **Test frequently** - Run tests after each feature implementation
2. **Use --stop-on-fail** during development to catch issues early
3. **Keep the 100-byte buffer** - It exposes real edge cases
4. **Add tests for new features** - Keep test suite comprehensive
5. **Check test output** - Don't just look at pass/fail, read the messages

## Notes

- Tests use a **100-byte send buffer** to test chunked responses and expose edge cases
- Some tests may be skipped if features aren't implemented yet - that's expected!
- The test suite is designed to be **strict** - it validates HTTP/1.1 compliance
- Tests assume server is already running (except `make test-full`)

## Contributing

When adding new tests:
1. Follow the existing pattern (see test_basic.cpp)
2. Use the assertion helpers for consistent output
3. Handle connection failures gracefully (add_skip)
4. Add clear test names and messages
5. Update this README

---

**Happy Testing! 🚀**
