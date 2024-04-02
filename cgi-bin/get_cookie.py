#! /usr/bin/python3

import os
from http import cookies
import cgi

form = cgi.FieldStorage()

key = form.getvalue("key")
cookie = cookies.SimpleCookie()

if "HTTP_COOKIE" in os.environ:
	cookie.load(os.environ["HTTP_COOKIE"])
else:
	cookie = None

page = """\
HTTP/1.1 200 OK
Content-Type: text/html\r

<html lang="en">
	<head>
		<title>Get cookie</title>
	</head>
	<body>
"""


if cookie and key in cookie:
	page += f"		The value of cookie {key} is {cookie[key].value}\n"
else:
	page += "		Cookie was not found !\n"

page += """\
	</body>
</html>"""

print(page)
