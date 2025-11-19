#!/bin/bash

# Clean up any previous test artifacts
rm -f /tmp/server_pipeline.log
rm -f test_pipeline

# Compile the test
echo "Compiling test_pipeline..."
g++ -std=c++98 -Wall -Wextra -Werror test_pipeline.cpp -o test_pipeline
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

# Start server in background
echo "Starting server..."
cd ..
./webserv default.conf > /tmp/server_pipeline.log 2>&1 &
SERVER_PID=$!
echo "Server PID: $SERVER_PID"
cd test

# Give server time to start
sleep 1

# Run the test
echo ""
echo "Running pipeline test..."
echo "======================================"
./test_pipeline
TEST_RESULT=$?

# Kill server
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

# Show server log
echo ""
echo "======================================"
echo "SERVER LOG:"
echo "======================================"
cat /tmp/server_pipeline.log

exit $TEST_RESULT
