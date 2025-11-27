# CGI Testing Guide

## 📁 CGI Test Scripts Created

All scripts are in `data/cgi/` and are executable:

### 1. **hello.py** - Simple GET Test
- **Purpose**: Basic CGI functionality test
- **Method**: GET
- **Response**: "Hello from CGI!"
- **Use**: Verify basic CGI execution works

### 2. **echo.py** - POST Echo (Reversed)
- **Purpose**: Test POST data handling
- **Method**: POST
- **Input**: Any text
- **Response**: Input text reversed
- **Use**: Verify CGI receives and processes POST data

### 3. **uppercase.py** - POST Transformation
- **Purpose**: Test data processing
- **Method**: POST
- **Input**: Any text
- **Response**: Uppercase version
- **Use**: Verify CGI can transform data

### 4. **json_test.py** - JSON Response
- **Purpose**: Test JSON content type
- **Method**: POST
- **Input**: Any data
- **Response**: JSON object with metadata
- **Use**: Verify Content-Type handling

### 5. **large.py** - Large Output
- **Purpose**: Test chunked reading of CGI output
- **Output**: 10KB of data
- **Use**: Verify server reads large CGI output in chunks

### 6. **slow.py** - Slow Response
- **Purpose**: Test non-blocking CGI
- **Delay**: 2 seconds
- **Use**: Verify server doesn't block on slow CGI

### 7. **error.py** - Error Handling
- **Purpose**: Test CGI error handling
- **Exit code**: 1 (failure)
- **Use**: Verify server returns 500 on CGI failure

## 🧪 Running CGI Tests

### Compile and Run All CGI Tests
```bash
cd test/
make test-cgi
```

### Run Specific Tests with curl

**Simple GET:**
```bash
curl -v http://localhost:8080/cgi/hello.py
```

**POST with data:**
```bash
curl -v -X POST http://localhost:8080/cgi/uppercase.py \
  -H "Content-Type: text/plain" \
  -d "hello world"
```

**JSON response:**
```bash
curl -v -X POST http://localhost:8080/cgi/json_test.py \
  -H "Content-Type: application/json" \
  -d "test data"
```

**Test slow response:**
```bash
time curl http://localhost:8080/cgi/slow.py
# Should take ~2 seconds
```

**Test error handling:**
```bash
curl -v http://localhost:8080/cgi/error.py
# Should return 500 Internal Server Error
```

## ✅ Test Coverage

The test suite (`test_cgi.cpp`) includes **8 comprehensive tests**:

1. ✓ **Simple GET Request** - Basic CGI execution
2. ✓ **POST Echo (Reverse)** - POST data handling
3. ✓ **POST Uppercase** - Data transformation
4. ✓ **Error Handling** - CGI exit code != 0
5. ✓ **Large Output** - 10KB output (chunked reading)
6. ✓ **Slow Response** - 2-second delay (non-blocking)
7. ✓ **JSON Output** - Content-Type verification
8. ✓ **Keep-Alive** - Connection persistence after CGI

## 📊 Expected Test Results

When your CGI implementation is complete, you should see:

```
[CGI EXECUTION]
==========================================================
TEST: CGI: Simple GET Request
✓ PASS: Status 200
✓ PASS: CGI response correct

TEST: CGI: POST Echo (Reverse)
✓ PASS: Status 200
✓ PASS: CGI echoed and reversed POST data

TEST: CGI: POST Uppercase Conversion
✓ PASS: Status 200
✓ PASS: CGI converted to uppercase

TEST: CGI: Error Handling (Exit Code 1)
✓ PASS: Server returned 500 for failed CGI

TEST: CGI: Large Output (10KB)
✓ PASS: Status 200
✓ PASS: Received large CGI output

TEST: CGI: Slow Response (2 seconds)
✓ PASS: Status 200
✓ PASS: CGI slow response handled correctly

TEST: CGI: JSON Output
✓ PASS: Status 200
✓ PASS: CGI JSON response correct

TEST: CGI: Keep-Alive After CGI Request
✓ PASS: Status 200
✓ PASS: Keep-alive works with CGI
```

## 🐛 Debugging Tips

### 1. Check CGI Script Execution
```bash
# Test script directly
python3 data/cgi/hello.py
# Should output CGI headers + body
```

### 2. Check Server Logs
Look for error messages in your server output when CGI fails

### 3. Verbose Test Output
```bash
cd test/
./test_suite --only cgi --verbose
```

### 4. Test Individual Scripts
```bash
curl -v http://localhost:8080/cgi/hello.py 2>&1 | less
```

## 🔧 Common Issues

### Issue: "No such file or directory"
**Cause**: Wrong path to CGI script in `setup_cgi_request()`
**Fix**: Ensure path includes `/data/cgi/`

### Issue: "Permission denied"
**Cause**: Scripts not executable
**Fix**: `chmod +x data/cgi/*.py`

### Issue: Constant POLLIN triggers
**Cause**: Child process exits immediately (execve failed)
**Fix**: Check stderr output, verify Python path

### Issue: Tests hang
**Cause**: Not reading until EOF
**Fix**: Check `revents & POLLHUP` condition

## 🎯 Next Steps

1. **Fix the POLLHUP condition** if not done:
   ```cpp
   (fds["poll_fds"][i].revents & (POLLIN | POLLHUP))
   ```

2. **Run the tests**:
   ```bash
   cd test/
   make test-cgi
   ```

3. **Check results** and debug failures

4. **Add more scripts** if needed for your specific use cases

Good luck with your CGI implementation! 🚀
