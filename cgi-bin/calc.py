#! /usr/bin/python3

import cgi
import sys
from logger import *

form = cgi.FieldStorage()

page = """\
HTTP/1.1 200 OK
Content-type: text/html\r

<!DOCTYPE html>
<html lang="en">
	<head>
		<title>Calculator</title>
	</head>
	<body style="text-align: center;">
"""

expression = form.getvalue('expression')

if expression:
	expression = expression.replace('"', '').replace(' ', '+')

	log(f"Evaluating expression: \"{expression}\"", color = BLUE, file = sys.stderr)
	try:
		res = eval(expression)
		page += f"		<b>{expression} = {res}</b> <br>\n"
	except Exception as e:
		page += f"		<b>Something went wrong with expression \"{expression}\": {str(e)}</b> <br>\n"
else:
	page += "		<b>Expression is empty, please specify it.</b> <br>\n"

page += """\
	</body>
</html>"""

print(page)