#!/usr/bin/python3
import os, sys

method = os.environ.get("REQUEST_METHOD", "UNKNOWN")

print("Content-Type: text/plain\r\n\r\n")
print(f"Method received: {method}")

if method == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    data = sys.stdin.read(content_length) if content_length > 0 else ""
    print(f"\nPOST data: {data}")
elif method == "DELETE":
    print("\nDELETE method received successfully.")
else:
    print("\nThis endpoint supports GET, POST, and DELETE.")