#!/usr/bin/env python3
"""
Simple GET CGI script
Returns a greeting message
"""

def main():
    response = "Hello from CGI!"

    # CGI must output HTTP headers
    print("Content-Type: text/plain")
    print("Content-Length: " + str(len(response)))
    print()  # Empty line separates headers from body
    print(response, end='')

if __name__ == "__main__":
    main()
