#!/usr/bin/env python3
import os, sys

method = os.environ.get("REQUEST_METHOD", "GET")
print("Content-Type: application/json\r\n\r\n")

if method == "GET":
    print('{"result": "GET received"}')
elif method == "POST":
    body = sys.stdin.read()
    print(f'{{"result": "POST received", "body": "{body}"}}')
elif method == "DELETE":
    print('{"result": "DELETE received"}')
else:
    print(f'{{"error": "Unsupported method {method}"}}')