#!/usr/bin/env python3
"""
CGI script that returns large output (10KB)
Tests chunked reading of CGI output
"""

def main():
    # Generate 10KB of data
    line = "This is line of test data for large CGI output.\n"
    # 10KB = 10240 bytes, each line is ~48 bytes, so ~213 lines
    num_lines = 10240 // len(line)

    response = line * num_lines

    print("Content-Type: text/plain")
    print("Content-Length: " + str(len(response)))
    print()
    print(response, end='')

if __name__ == "__main__":
    main()
