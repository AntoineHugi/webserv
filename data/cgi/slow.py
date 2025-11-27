#!/usr/bin/env python3
"""
Slow CGI script - takes 2 seconds
Used to test that server doesn't block on slow CGI
"""
import time

def main():
    # Sleep for 2 seconds
    time.sleep(2)

    response = "Slow response after 2 seconds"
    print("Content-Type: text/plain")
    print("Content-Length: " + str(len(response)))
    print()
    print(response, end='')

if __name__ == "__main__":
    main()
