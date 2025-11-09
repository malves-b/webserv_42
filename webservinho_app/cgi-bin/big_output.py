#!/usr/bin/python3
print("Content-Type: text/plain\r\n\r\n")
print("Generating large output...\n")
for i in range(1000):
    print(f"Line {i+1}: This is a test of large CGI output.")