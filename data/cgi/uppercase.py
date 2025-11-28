#!/usr/bin/env python3
"""
POST CGI script - converts input to uppercase
"""
import sys

def main():
    # Read POST data from stdin
    input_data = sys.stdin.read()

    if input_data:
        # Convert to uppercase
        output = input_data.upper()
        # print("Content-Type: text/plain")
        # print("Content-Length: " + str(len(output)))
        # print()
        print(output, end='')
    else:
        response = "ERROR: No input data"
        # print("Status: 400 Bad Request")
        # print("Content-Type: text/plain")
        # print("Content-Length: " + str(len(response)))
        # print()
        print(response, end='')

if __name__ == "__main__":
    main()
