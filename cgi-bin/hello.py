#! /usr/bin/python3

import cgi

form = cgi.FieldStorage()

first_name = form.getvalue('first_name')
last_name = form.getvalue('last_name')

page = f"""\
HTTP/1.1 200 OK
Content-type: text/html\r

<!DOCTYPE html>
<html lang="en">
	<head>
		<title>Hello CGI</title>
	</head>
	<body>
		<h2>Hello {first_name or "John"} {last_name or "Doe"}</h2>
	</body>
</html>
"""

print(page)