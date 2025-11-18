# Testing Concurrent Clients

## The Problem with Sequential Testing

When you test like this:
```bash
curl http://localhost:8080/
curl http://localhost:8080/index
curl http://localhost:8080/data
```

You're testing **one client sending multiple requests**, not **multiple clients at the same time**.

Your server only experiences:
- One socket at a time
- No interleaving of requests
- No race conditions
- No resource contention

## How to Test TRUE Concurrency

### Method 1: Built-in Test Suite (Recommended)

The updated `test_client` now includes a concurrent test using `fork()`:

```bash
# Compile tests
make -f Makefile.test

# Run full test suite (includes concurrent test)
./test_client
```

Look for this test:
```
==========================================================
TEST: Truly Concurrent Clients (5 clients via fork, 3 requests each)
==========================================================
ℹ INFO: Spawning 5 concurrent client processes...
✓ PASS: 5/5 concurrent clients completed all requests
ℹ INFO: All concurrent processes completed
```

**What this does:**
- Spawns 5 separate **processes** (via fork)
- Each process connects simultaneously
- Each sends 3 requests on same connection (keep-alive)
- Total: 15 requests from 5 concurrent clients

### Method 2: Dedicated Stress Test

For heavier load testing:

```bash
# Compile
make -f Makefile.test

# Run with defaults (10 clients, 5 requests each = 50 total)
./stress_test

# Custom parameters
./stress_test 20 10   # 20 clients, 10 requests each = 200 total
./stress_test 50 3    # 50 clients, 3 requests each = 150 total
./stress_test 100 1   # 100 clients, 1 request each = 100 total
```

**Example output:**
```
==========================================================
  CONCURRENT CLIENT STRESS TEST
==========================================================
Clients:            20
Requests per client:10
Total requests:     200
==========================================================

Spawning 20 concurrent client processes...
  Spawned client 0 (PID: 12345)
  Spawned client 1 (PID: 12346)
  ...

All clients spawned. Waiting for completion...
  ✓ Client 0 completed successfully
  ✓ Client 1 completed successfully
  ...

==========================================================
  RESULTS
==========================================================
Successful clients: 20/20
Failed clients:     0/20
Total requests:     200

  ✓✓✓ ALL CLIENTS SUCCEEDED! ✓✓✓
```

---

## What Your Server Experiences

### Sequential Testing (curl one at a time):
```
Time →

Client A: ──[connect]──[request]──[response]──[close]──
                                                      Client B: ──[connect]──[request]──...
```

### Concurrent Testing (fork-based):
```
Time →

Client A: ──[connect]──[req1]──[req2]──[req3]──[close]──
Client B:    ──[connect]──[req1]──[req2]──[req3]──[close]──
Client C:       ──[connect]──[req1]──[req2]──[req3]──[close]──
Client D:          ──[connect]──[req1]──[req2]──[req3]──[close]──
Client E:             ──[connect]──[req1]──[req2]──[req3]──[close]──
```

**Your server must handle:**
- Multiple sockets in poll_fds simultaneously
- Some clients reading, some writing
- Some connecting, some disconnecting
- Interleaved POLLIN/POLLOUT events

---

## Manual Testing Alternatives

### Using xargs (shell command)
```bash
# 10 concurrent curl requests
seq 10 | xargs -P 10 -I {} curl -s http://localhost:8080/test{} > /dev/null
```

### Using GNU parallel
```bash
# Install: sudo apt-get install parallel
parallel -j 20 curl -s ::: http://localhost:8080/{1..20}
```

### Using ab (Apache Bench)
```bash
# Install: sudo apt-get install apache2-utils
ab -n 100 -c 10 http://localhost:8080/
# -n 100 = 100 total requests
# -c 10  = 10 concurrent clients
```

### Using background jobs
```bash
# Start 5 clients in background
for i in {1..5}; do
    (curl http://localhost:8080/client$i && echo "Client $i done") &
done
wait  # Wait for all background jobs
```

---

## What to Look For in Your Server Logs

### Good Signs:
```
New connection!
 Client 5 connected. Total clients: 1
New connection!
 Client 6 connected. Total clients: 2
New connection!
 Client 7 connected. Total clients: 3
Client is sending data   # From client 5
Client is sending data   # From client 6
Client is sending data   # From client 5 again
Client is sending data   # From client 7
```

Notice: **Requests interleave** - server handles them as they come.

### Bad Signs:
```
New connection!
 Client 5 connected. Total clients: 1
Client 5 closed connection
New connection!
 Client 5 connected. Total clients: 1  # ← Same FD reused!
Old request data appears!               # ← BUG: not erased from map
```

Or:
```
New connection!
 Client 5 connected. Total clients: 1
Segmentation fault                     # ← Vector iterator invalidation
```

---

## Common Bugs Revealed by Concurrent Testing

### 1. Use-After-Erase
```cpp
// BAD
poll_fds.erase(poll_fds.begin() + i);
clients.erase(poll_fds[i].fd);  // ← poll_fds[i] doesn't exist!

// GOOD
int fd = poll_fds[i].fd;
poll_fds.erase(poll_fds.begin() + i);
clients.erase(fd);
```

### 2. FD Reuse with Old Data
```cpp
// Old client 5 disconnects
// New client connects and gets FD 5 (OS reuses FDs)
// clients[5] still has old data because erase() failed

// FIX: Ensure clients.erase(fd) always runs
```

### 3. new_client Flag Not Reset
```cpp
int new_client = 0;
for (...) {
    if (is_server_fd)
        new_client = 1;  // Set

    if (new_client)      // ← Stays 1 for all remaining FDs!
        accept_client();

    // FIX: new_client = 0; at start of loop
}
```

### 4. Vector Invalidation in Forward Loop
```cpp
for (size_t i = 0; i < poll_fds.size(); i++) {
    if (error)
        poll_fds.erase(i);  // ← Size changes, i++ skips next element
}

// FIX: Loop backwards or adjust i after erase
```

---

## Recommended Test Progression

1. **Start Small**
   ```bash
   ./stress_test 2 1    # 2 clients, 1 request each
   ```

2. **Increase Clients**
   ```bash
   ./stress_test 5 1
   ./stress_test 10 1
   ./stress_test 20 1
   ```

3. **Add Keep-Alive**
   ```bash
   ./stress_test 5 3    # 5 clients, 3 requests each
   ./stress_test 10 5
   ```

4. **Heavy Load**
   ```bash
   ./stress_test 50 10   # 500 total requests
   ./stress_test 100 5   # 500 total requests
   ```

5. **Extreme Stress**
   ```bash
   ./stress_test 200 10  # 2000 requests
   # Does it crash? Leak FDs? Slow down?
   ```

---

## Monitoring Your Server

### Check for FD leaks:
```bash
# In another terminal while stress test runs:
watch -n 1 'lsof -p $(pgrep webserv) | wc -l'
```

If the number keeps growing → FD leak!

### Check memory:
```bash
watch -n 1 'ps aux | grep webserv'
```

If memory (RSS) keeps growing → Memory leak!

### Check connections:
```bash
netstat -an | grep :8080 | grep ESTABLISHED
```

Should show active connections during test.

---

## Success Criteria

✅ **All concurrent clients complete successfully**
✅ **No crashes or segfaults**
✅ **No FD leaks** (FD count returns to baseline)
✅ **No memory leaks**
✅ **Logs show interleaved requests** (not sequential)
✅ **Performance doesn't degrade** (stress test with 100 clients works)

If all these pass → Your concurrent handling is solid! 🎉

---

## Next: What Concurrent Testing WON'T Catch

This testing focuses on **I/O multiplexing**. You still need to test:
- **Large files** (chunked transfer, phased sends)
- **Slow clients** (timeout handling)
- **Malicious clients** (huge headers, incomplete requests)
- **CGI execution** (fork, exec, pipes)
- **Error conditions** (file not found, permissions, etc.)

But for validating your `poll_service` logic → concurrent testing is essential!
