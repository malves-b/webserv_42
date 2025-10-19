#!/usr/bin/env python3
print("Content-Type: text/html\r\n\r\n")
print("""
<html>
  <body>
    <h1>Form Test</h1>
    <form action="/cgi-bin/echo.py" method="post">
      Nome: <input type="text" name="nome"><br>
      Mensagem: <input type="text" name="msg"><br>
      <input type="submit" value="Enviar">
    </form>
  </body>
</html>
""")