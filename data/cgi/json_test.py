#!/usr/bin/env python3
"""
CGI script that returns JSON
Reads POST data and returns it as JSON with metadata
"""
import sys
import json

def main():
    # Read POST data
    input_data = sys.stdin.read()

    # Create JSON response
    response_data = {
        "received": input_data,
        "length": len(input_data),
        "type": "echo",
        "status": "success"
    }

    response = json.dumps(response_data)

    print("Content-Type: application/json")
    print("Content-Length: " + str(len(response)))
    print()
    print(response, end='')

if __name__ == "__main__":
    main()
