#!/usr/bin/env python3
"""
CGI script that returns environment variables as JSON
Used to test that server correctly sets CGI environment variables
"""
import os
import json

def main():
    # Collect all relevant CGI environment variables
    env_data = {
        "REQUEST_METHOD": os.environ.get("REQUEST_METHOD", ""),
        "CONTENT_TYPE": os.environ.get("CONTENT_TYPE", ""),
        "CONTENT_LENGTH": os.environ.get("CONTENT_LENGTH", ""),
        "QUERY_STRING": os.environ.get("QUERY_STRING", ""),
        "SERVER_PROTOCOL": os.environ.get("SERVER_PROTOCOL", ""),
        "SERVER_PORT": os.environ.get("SERVER_PORT", ""),
        "SERVER_NAME": os.environ.get("SERVER_NAME", ""),
        "GATEWAY_INTERFACE": os.environ.get("GATEWAY_INTERFACE", ""),
        "SCRIPT_NAME": os.environ.get("SCRIPT_NAME", ""),
        "PATH_INFO": os.environ.get("PATH_INFO", ""),
        "REMOTE_ADDR": os.environ.get("REMOTE_ADDR", ""),
        # HTTP headers
        "HTTP_USER_AGENT": os.environ.get("HTTP_USER_AGENT", ""),
        "HTTP_HOST": os.environ.get("HTTP_HOST", ""),
        "HTTP_ACCEPT": os.environ.get("HTTP_ACCEPT", "")
    }

    response = json.dumps(env_data, indent=2)
    print(response, end='')

if __name__ == "__main__":
    main()
