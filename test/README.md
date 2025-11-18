# Webserv Test Suite

A comprehensive, modular test suite for the HTTP/1.1 webserv server implementation.

## Features

- **Modular Design**: Tests organized into independent categories
- **Selective Execution**: Run specific test categories or skip unwanted ones
- **Color-Coded Output**: Easy-to-read results with colored pass/fail indicators
- **C++98 Compatible**: Full compatibility with the project's C++ standard
- **Comprehensive Coverage**: Tests for basic connectivity, keep-alive, chunked encoding, and pipelining

## Quick Start

### Build the Test Suite

```bash
cd test/
make -f Makefile.tests
```

### Run All Tests

```bash
./test_suite
```

### Run Specific Categories

```bash
./test_suite --only basic
./test_suite --only keepalive
./test_suite --only chunked
./test_suite --only pipeline
```

### Skip Specific Categories

```bash
./test_suite --skip pipeline
./test_suite --skip basic --skip pipeline
```

### Other Options

```bash
./test_suite --help              # Show help message
./test_suite --list              # List available categories
./test_suite --verbose           # Enable verbose output
./test_suite --stop-on-fail      # Stop on first failure
./test_suite --host 127.0.0.1    # Specify server host
./test_suite --port 8080         # Specify server port
```

## Test Categories

### 1. Basic Connectivity & Requests (`basic`)

Tests fundamental HTTP server functionality:
- Basic TCP connection establishment
- Simple GET request handling
- 404 Not Found response
- HTTP/1.1 version compliance
- Required response headers (Date, Server, Content-Length)

**Current Status**: 8/9 tests passing (88.89%)

### 2. Connection Persistence (`keepalive`)

Tests HTTP/1.1 keep-alive connection behavior:
- Connection: keep-alive header support
- Multiple requests on same TCP connection
- Connection: close header handling
- Proper connection termination
- Idle timeout behavior (optional)

**Current Status**: 10/10 tests passing (100% - 1 skipped)

### 3. Chunked Transfer Encoding (`chunked`)

Tests chunked request/response handling:
- Simple chunked POST requests
- Multiple chunks in single request
- Chunked encoding with keep-alive connections
- Proper chunk size parsing
- Final chunk (0\r\n\r\n) handling

**Current Status**: 5/5 tests passing (100%)

### 4. Pipelined Requests (`pipeline`)

Tests HTTP/1.1 pipelining support:
- Chunked POST followed by GET in single TCP send
- Content-Length POST followed by GET
- Three sequential GET requests
- Leftover data handling between requests
- Response ordering preservation

**Current Status**: 2/3 tests passing (66.67%)

## Test Results Summary

Latest test run:
- **Total Tests**: 23
- **Passed**: 20 (86.96%)
- **Failed**: 2
- **Skipped**: 1

### Known Issues

1. **404 Not Found Test Failing**: Server returns 200 for non-existent files instead of 404
2. **Three Sequential Pipelined GETs**: Only 2 out of 3 requests are processed

These failures indicate incomplete server features that need to be implemented.

## Architecture

### File Structure

```
test/
├── test_framework.hpp       # Core framework: structs, utilities, declarations
├── test_framework.cpp       # Implementation: networking, parsing, assertions
├── test_basic.cpp           # Basic connectivity tests
├── test_keepalive.cpp       # Keep-alive connection tests
├── test_chunked.cpp         # Chunked transfer encoding tests
├── test_pipelining.cpp      # Pipelined request tests
├── test_suite.cpp           # Main runner with CLI parsing
├── Makefile.tests           # Build system
└── README.md                # This file
```

### Key Components

**TestConfig**: Configuration struct with server host/port, enabled categories, verbosity

**TestStats**: Tracks test results (total, passed, failed, skipped) with pass rate calculation

**HttpResponse**: Parsed HTTP response with status line, headers, body, and status code

**Color Namespace**: ANSI color codes for formatted output

**Assertion Functions**:
- `assert_status()` - Verify HTTP status code
- `assert_header()` - Verify header value
- `assert_header_exists()` - Verify header presence
- `assert_body_contains()` - Verify body content
- `assert_connection_closed()` - Verify socket closed

## Adding New Tests

### 1. Create a New Test Function

```cpp
void test_my_new_feature(const TestConfig& config, TestStats& stats) {
    print_test("My New Feature");

    int sock = connect_to_server(config.server_host.c_str(), config.server_port);
    if (sock < 0) {
        stats.add_skip();
        print_skip("Connection failed");
        return;
    }

    std::map<std::string, std::string> headers;
    std::string response = send_request(sock, "GET", "/path", headers);
    HttpResponse resp = parse_response(response);

    assert_status(resp, 200, stats);

    close(sock);
}
```

### 2. Add to Category Runner

```cpp
void run_my_category_tests(const TestConfig& config, TestStats& stats) {
    print_category("MY NEW CATEGORY");

    test_my_new_feature(config, stats);
    // Add more tests...
}
```

### 3. Register in test_suite.cpp

Add forward declaration and call in main():
```cpp
void run_my_category_tests(const TestConfig& config, TestStats& stats);

// In main():
if (config.enabled_categories["mycategory"]) {
    run_my_category_tests(config, stats);
}
```

## Testing Best Practices

1. **Connection Management**: Always close sockets when done
2. **Error Handling**: Use skip status when connection fails
3. **Assertions**: Use provided assertion helpers for consistent output
4. **Descriptive Names**: Use clear, descriptive test and category names
5. **Isolated Tests**: Each test should be independent
6. **Cleanup**: Ensure proper cleanup even on test failure

## Troubleshooting

### Server Not Running

```
✗ ERROR: Cannot connect to server at 127.0.0.1:8080
Make sure your server is running!
```

**Solution**: Start the webserv server before running tests:
```bash
./webserv default.conf
```

### Connection Timeout

Tests hang or timeout waiting for responses.

**Solution**: Check server logs for errors, ensure proper request handling and response generation.

### Compilation Errors

**Solution**: Ensure all source files are present and Makefile paths are correct:
```bash
make -f Makefile.tests clean
make -f Makefile.tests
```

## Future Enhancements

Potential additions to the test suite:

1. **CGI Tests**: Execute CGI scripts and validate output
2. **Method Tests**: Comprehensive POST, DELETE, PUT testing
3. **Error Code Tests**: All HTTP error codes (400, 403, 413, 500, 501, etc.)
4. **Large File Tests**: Upload and download of large files
5. **Concurrent Tests**: Multiple simultaneous connections
6. **Malformed Request Tests**: Handling of invalid HTTP requests
7. **Configuration-Driven Tests**: Parse server config to determine available routes/methods
8. **Performance Tests**: Response time and throughput measurements
9. **Stress Tests**: Server behavior under heavy load
10. **Security Tests**: Common web vulnerabilities (XSS, injection, etc.)

## Contributing

When adding new tests:
1. Follow the existing code style and structure
2. Ensure C++98 compatibility (no C++11/14/17 features)
3. Add appropriate documentation and comments
4. Update this README with new test categories
5. Test compilation and execution before committing

## License

Part of the 42 School webserv project.
