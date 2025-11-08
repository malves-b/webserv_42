#!/usr/bin/python3
import os, cgi

print("Content-Type: text/html\r\n\r\n")
print("<html><body><h2>Python CGI Echo</h2>")
print("<p><b>Query string:</b> {}</p>".format(os.environ.get("QUERY_STRING", "")))

form = cgi.FieldStorage()
if form:
    print("<ul>")
    for key in form.keys():
        print(f"<li>{key}: {form.getvalue(key)}</li>")
    print("</ul>")

print("<hr><p>Environment Variables:</p><pre>")
for k, v in sorted(os.environ.items()):
    print(f"{k}={v}")
print("""<a href="/">Back to Home</a>""")
print("</pre></body></html>")
