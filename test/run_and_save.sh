#!/bin/bash

# Script to build, run tests, and save results to a file

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUTPUT_FILE="test_results_${TIMESTAMP}.txt"

echo "==================================="
echo "  Webserv Test Runner with Output"
echo "==================================="
echo
echo "Output will be saved to: ${OUTPUT_FILE}"
echo

# Check if webserv is running
if ! nc -z localhost 8080 2>/dev/null; then
    echo "❌ Error: Server not running on port 8080" | tee -a "${OUTPUT_FILE}"
    echo "Please start your server first with: ./webserv default.conf" | tee -a "${OUTPUT_FILE}"
    exit 1
fi

echo "✓ Server detected on port 8080" | tee -a "${OUTPUT_FILE}"
echo | tee -a "${OUTPUT_FILE}"

# Build test client
echo "Building test client..." | tee -a "${OUTPUT_FILE}"
make -f Makefile.test re 2>&1 | tee -a "${OUTPUT_FILE}"

if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "❌ Failed to build test client" | tee -a "${OUTPUT_FILE}"
    exit 1
fi

echo | tee -a "${OUTPUT_FILE}"
echo "Running tests..." | tee -a "${OUTPUT_FILE}"
echo | tee -a "${OUTPUT_FILE}"

# Run tests and save output
./test_client 2>&1 | tee -a "${OUTPUT_FILE}"
TEST_RESULT=${PIPESTATUS[0]}

echo | tee -a "${OUTPUT_FILE}"
echo "==================================="  | tee -a "${OUTPUT_FILE}"
echo "Tests complete. Results saved to: ${OUTPUT_FILE}" | tee -a "${OUTPUT_FILE}"
echo "==================================="  | tee -a "${OUTPUT_FILE}"

# Clean up
make fclean -f Makefile.test > /dev/null 2>&1

exit ${TEST_RESULT}
