# Webserv Test Suite

Comprehensive testing tools for the webserv HTTP server implementation.

## Quick Start

### Terminal 1: Start Server
```bash
cd ..
make re
./webserv default.conf
```

### Terminal 2: Run Tests
```bash
cd test
./run_tests.sh
```

---

## Test Files

| File | Purpose |
|------|---------|
| **test_client.cpp** | Main test suite - tests all basic functionality |
| **stress_test.cpp** | Concurrent load testing |
| **Makefile.test** | Builds both test programs |
| **run_tests.sh** | One-command test runner |
| **TEST_GUIDE.md** | Detailed test documentation |
| **CONCURRENT_TESTING.md** | Guide to concurrent client testing |

---

## Running Tests

### 1. Full Test Suite
```bash
cd test
./run_tests.sh
```

Tests included:
- ✓ Basic connection
- ✓ Keep-alive (multiple requests, same connection)
- ✓ Connection: close behavior
- ✓ HTTP methods (GET, POST, DELETE, PUT)
- ✓ POST with body
- ✓ Rapid connections
- ✓ **Truly concurrent clients** (fork-based)
- ✓ Multiple server ports
- ✓ Large headers
- ✓ **Chunked transfer encoding**
- ✓ **Pipelined requests with chunked encoding**
- ✓ **Pipelined requests with Content-Length**

### 2. Manual Test Run
```bash
cd test
make -f Makefile.test
./test_client
```

### 3. Stress Testing
```bash
cd test
make -f Makefile.test

# Default: 10 clients, 5 requests each
./stress_test

# Custom load
./stress_test 20 10    # 20 clients × 10 requests = 200 total
./stress_test 50 5     # 50 clients × 5 requests = 250 total
./stress_test 100 10   # 100 clients × 10 requests = 1000 total
```

---

## Expected Output

### Success:
```
==========================================================
  TEST SUMMARY
==========================================================
Passed: 30
Failed: 0
Total:  30

==========================================================
  ALL TESTS PASSED! ✓
==========================================================
```

### Stress Test Success:
```
==========================================================
  RESULTS
==========================================================
Successful clients: 50/50
Failed clients:     0/50
Total requests:     500

  ✓✓✓ ALL CLIENTS SUCCEEDED! ✓✓✓
```

---

## Troubleshooting

### Server not running
```
❌ Error: Server not running on port 8080
```
**Fix:** Start server in another terminal: `./webserv default.conf`

### Compilation errors
```bash
cd test
make -f Makefile.test fclean
make -f Makefile.test
```

### Tests hanging
- Check if server is responsive: `curl http://localhost:8080/`
- Check server logs for errors
- Restart server

### Some tests fail
- Read `TEST_GUIDE.md` for test descriptions
- Check server output for errors
- Look for common issues in `CONCURRENT_TESTING.md`

---

## Documentation

- **TEST_GUIDE.md** - Detailed explanation of each test
- **CONCURRENT_TESTING.md** - How concurrent testing works and what to look for

---

## Cleaning Up

```bash
# Clean test binaries
cd test
make -f Makefile.test fclean

# Or manually
rm -f test_client stress_test *.o
```

---

## CI/CD Integration

You can integrate these tests into your workflow:

```bash
#!/bin/bash
# Simple CI script

# Build server
make re || exit 1

# Start server in background
./webserv default.conf &
SERVER_PID=$!

# Wait for server to start
sleep 2

# Run tests
cd test
./run_tests.sh
TEST_RESULT=$?

# Stop server
kill $SERVER_PID

# Exit with test result
exit $TEST_RESULT
```

---

## Next Steps

After all tests pass:
1. Test with actual file serving (not mocked responses)
2. Test chunked transfer encoding
3. Test CGI execution
4. Test error pages
5. Test with real browsers

Good luck! 🚀
