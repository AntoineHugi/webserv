#!/usr/bin/env python3
"""
Simple echo CGI script
Reads from stdin and writes it back reversed
"""
import sys

def main():
    # Read POST data from stdin
    content_length = sys.stdin.read()

    if content_length:
        # Echo back reversed
        reversed_data = content_length[::-1]
        print("Content-Type: text/plain")
        print("Content-Length: " + str(len(reversed_data)))
        print()  # Empty line separates headers from body
        print(reversed_data, end='')
    else:
        # No input
        response = "No data received"
        print("Content-Type: text/plain")
        print("Content-Length: " + str(len(response)))
        print()
        print(response, end='')

if __name__ == "__main__":
    main()
