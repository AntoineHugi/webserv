#!/bin/bash

# Script to build and run the test suite

echo "==================================="
echo "  Webserv Test Runner"
echo "==================================="
echo

# Check if webserv is running
if ! nc -z localhost 8080 2>/dev/null; then
    echo "❌ Error: Server not running on port 8080"
    echo "Please start your server first with: ./webserv default.conf"
    exit 1
fi

echo "✓ Server detected on port 8080"
echo

# Build test client
echo "Building test client..."
make -f Makefile.test re

if [ $? -ne 0 ]; then
    echo "❌ Failed to build test client"
    exit 1
fi

echo
echo "Running tests..."
echo

# Run tests
./test_client
make fclean -f Makefile.test
exit $?
