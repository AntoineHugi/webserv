# Webserv Test Suite Guide

## Quick Start

### Terminal 1: Start your server
```bash
make re
./webserv default.conf
```

### Terminal 2: Run tests
```bash
./run_tests.sh
```

Or manually:
```bash
make -f Makefile.test
./test_client
```

---

## Test Descriptions

### TEST 1: Basic Connection and Response
**What it tests:**
- Can connect to server
- Server responds with valid HTTP
- Receives 200 OK status
- Connection closes properly

**Expected output:**
```
✓ Connected to server
✓ Received 200 OK response
✓ Connection closed cleanly
```

**What can go wrong:**
- Server not running → Connection fails
- Invalid HTTP response → Parse error
- Server crashes → No response

---

### TEST 2: Keep-Alive - Multiple Sequential Requests
**What it tests:**
- Multiple requests on same TCP connection
- Server sends `Connection: keep-alive` header
- All requests get 200 OK
- Socket stays open between requests

**Expected output:**
```
✓ Connected to server
✓ First request: 200 OK
✓ Server responded with Connection: keep-alive
✓ Second request on same connection: 200 OK
✓ Third request on same connection: 200 OK
✓ All 3 requests completed on single connection
```

**What can go wrong:**
- Server closes after first response → Connection refused on 2nd request
- Server sends `Connection: close` → Client must reconnect
- Socket FD reused incorrectly → Old data appears

---

### TEST 3: Connection Close Behavior
**What it tests:**
- Client sends `Connection: close`
- Server responds with `Connection: close`
- Server closes connection after response
- Subsequent requests fail

**Expected output:**
```
✓ Received 200 OK
✓ Server responded with Connection: close
✓ Server closed connection after Connection: close
```

**What can go wrong:**
- Server ignores `Connection: close` → Stays open
- Server closes but doesn't send header → Client confused

---

### TEST 4: Different HTTP Methods
**What it tests:**
- GET, POST, DELETE, PUT methods
- Server handles all methods (even if not fully implemented)
- No crashes on different methods

**Expected output:**
```
✓ GET request successful
✓ POST request successful
✓ DELETE request successful
ℹ PUT returned: 200 (or other status)
```

**What can go wrong:**
- Server only accepts GET → Other methods fail
- Server crashes on unexpected method

---

### TEST 5: POST Request with JSON Body
**What it tests:**
- Request with Content-Length header
- Request with body data
- Content-Type header
- Body parsing

**Expected output:**
```
✓ POST with JSON body successful
```

**What can go wrong:**
- Server doesn't read body → Incomplete request
- Content-Length mismatch → Parse error
- Body not stored correctly

---

### TEST 6: Rapid Connect/Disconnect
**What it tests:**
- Server handles many quick connections
- No resource leaks
- FD management working
- No crashes under load

**Expected output:**
```
✓ 10/10 rapid connections successful
```

**What can go wrong:**
- FD leak → Eventually can't accept more connections
- Race condition in accept() → Some connections fail
- Server slows down → Timeouts

---

### TEST 7: Multiple Server Ports
**What it tests:**
- Configuration with multiple servers
- Each server listening on different port
- Both servers work independently

**Expected output:**
```
✓ Server on port 8080 responsive
✓ Server on port 8081 responsive
```

**What can go wrong:**
- Only first server starts → Port 8081 unreachable
- Servers share state incorrectly → Conflicts

---

### TEST 8: Large Header (within limits)
**What it tests:**
- Server accepts headers up to 16KB
- 10KB cookie header processed correctly
- No buffer overflow
- No crash

**Expected output:**
```
✓ Large header accepted (within limit)
```

**What can go wrong:**
- Buffer too small → Request rejected or crash
- Header size limit too strict → 431 error
- Parsing breaks on large headers

---

### TEST 10: Chunked Transfer Encoding
**What it tests:**
- Request with `Transfer-Encoding: chunked` header
- Multiple chunks with size prefixes
- Proper termination with `0\r\n\r\n`
- Body reassembly from chunks

**Expected output:**
```
✓ Chunked request processed successfully
```

**What can go wrong:**
- Server doesn't recognize chunked encoding → 400 Bad Request
- Chunks not parsed correctly → Body incomplete or malformed
- Missing `0\r\n\r\n` terminator → Server waits forever
- Chunk size parsing fails → Wrong body content

**Why this matters:**
Chunked encoding is essential for HTTP/1.1 compliance. It allows sending data without knowing Content-Length upfront (e.g., streaming, dynamic content, CGI output).

---

### TEST 11: Pipelined Requests with Chunked Encoding
**What it tests:**
- Client sends chunked POST + GET in one send() call
- Server detects leftover data after `0\r\n\r\n`
- Leftover data is processed as next request
- Both requests receive responses

**Expected output:**
```
✓ Both pipelined requests processed (chunked leftover detected)
```

**What can go wrong:**
- Server ignores data after `0\r\n\r\n` → Second request lost
- Leftover flag not set → Server waits for new recv() that never comes
- GET request treated as part of chunked body → Parse error

**Why this matters:**
With HTTP/1.1 keep-alive, clients can pipeline requests (send multiple without waiting for responses). If you don't preserve leftover data after chunked body, pipelined requests are lost.

**How it works:**
```
Client sends:
POST /upload HTTP/1.1
Transfer-Encoding: chunked

5
Hello
0

GET /next HTTP/1.1

```

After reading chunked POST (ends at `0\r\n\r\n`), the string `GET /next HTTP/1.1\r\n...` is leftover and should be processed as next request.

---

### TEST 12: Pipelined Requests with Content-Length
**What it tests:**
- Client sends POST (Content-Length) + GET in one send() call
- Server reads exactly Content-Length bytes for body
- Leftover data after body is next request
- Both requests receive responses

**Expected output:**
```
✓ Both pipelined requests processed (Content-Length leftover detected)
```

**What can go wrong:**
- Server ignores data after Content-Length bytes → Second request lost
- Server reads too much (beyond Content-Length) → GET parsed as body
- No leftover detection → Client waits forever for response to GET

**Why this matters:**
Even with Content-Length, clients can pipeline. After reading exactly `Content-Length` bytes, any remaining data in the buffer is the next request. Without leftover detection, pipelined requests after non-chunked POSTs fail.

**How it works:**
```
Client buffer contains:
POST /submit HTTP/1.1
Content-Length: 13

Hello, World!GET /after HTTP/1.1

```

After reading header + 13 bytes of body, `GET /after HTTP/1.1\r\n...` is leftover.

---

## Manual Tests (for deeper debugging)

### Test Keep-Alive with curl:
```bash
curl -v http://localhost:8080/ http://localhost:8080/index
```

Look for:
- Both requests use same connection
- `Connection: keep-alive` in response headers

### Test with telnet (interactive):
```bash
telnet localhost 8080

# Type manually:
GET / HTTP/1.1
Host: localhost:8080
Connection: keep-alive

[press Enter twice]

# After response, send another:
GET /test HTTP/1.1
Host: localhost:8080
Connection: close

[press Enter twice]
```

### Test concurrent connections:
```bash
# Terminal 1
curl http://localhost:8080/slow &

# Terminal 2 (immediately)
curl http://localhost:8080/fast

# Both should complete
```

### Test malformed request:
```bash
echo -e "INVALID REQUEST\r\n\r\n" | nc localhost 8080
# Server should not crash
```

---

## Debugging Tips

### Server prints "Client X closed connection" immediately:
- **Cause:** recv() returns 0 after sending response
- **Why:** You sent `Connection: close` but tried to keep socket open
- **Fix:** Check `_keep_alive` flag, close socket when appropriate

### Old request data appears in new request:
- **Cause:** clients[fd] not erased properly
- **Why:** Use-after-erase bug (accessing poll_fds[i].fd after erase)
- **Fix:** Save fd before erasing: `int fd = poll_fds[i].fd;`

### Second request on keep-alive fails:
- **Cause:** Response has `Connection: close` instead of `keep-alive`
- **Why:** format_response() hardcoded "close"
- **Fix:** Check Connection header in request, set _keep_alive flag

### Tests hang:
- **Cause:** poll() waiting forever on broken socket
- **Why:** Not handling POLLHUP or POLLERR
- **Fix:** Check for error events and close socket

### Random failures:
- **Cause:** Vector iterator invalidation
- **Why:** Erasing from vector while iterating forward
- **Fix:** Iterate backwards or adjust index after erase

---

## Expected Test Results

**Passing all tests means:**
✅ Keep-alive works correctly
✅ Connection management is solid
✅ No resource leaks
✅ Multiple clients handled
✅ Different HTTP methods supported
✅ Request parsing works
✅ No crashes on edge cases

**Common failure patterns:**
- 0/10 rapid connections → Server not running or crashed
- Keep-alive fails → Connection: close hardcoded
- Second request fails → Socket closed too early
- Random passes/fails → Race condition or use-after-free

---

## Next Steps After Passing Tests

1. **Add actual file serving** (currently mocked responses)
2. **Implement chunked transfer encoding** (for large files)
3. **Add timeout for idle connections**
4. **Implement pipelined requests** (client sends multiple without waiting)
5. **Add request routing** (match URI to configured locations)
6. **Implement CGI execution**
7. **Add error pages** (404, 500, etc.)

Good luck! 🚀
