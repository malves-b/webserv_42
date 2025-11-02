#!/usr/bin/env python3
import os, cgi, time

print("Content-Type: text/html\r\n\r\n")
print("<h1>Echo CGI</h1>")
print(f"<p>Method: {os.environ.get('REQUEST_METHOD')}</p>")
print(f"<p>Query: {os.environ.get('QUERY_STRING')}</p>")

form = cgi.FieldStorage()
if form:
    print("<h2>POST/FORM data:</h2>")
    time.sleep(100.0)
    for key in form.keys():
        print(f"<p>{key} = {form.getvalue(key)}</p>")
