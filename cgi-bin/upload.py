#! /usr/bin/python3

import cgi
import os
import sys
from logger import *

form = cgi.FieldStorage()

page = """\
HTTP/1.1 200 OK
Content-Type: text/html;charset=utf-8\r

<!DOCTYPE html>
<html lang="en">
	<head>
	<title>File upload</title>
	</head>
	<body>
"""

try:
	if not "filename" in form:
		raise Exception("Missing filename in form")
		
	fileitem = form["filename"]

	if not fileitem.filename:
		raise Exception("Uploading Failed")
		
	log("Received a file", color = BLUE, file = sys.stderr)

	if not os.path.exists(os.getcwd() + '/cgi-bin/tmp/'):
		os.mkdir(os.getcwd() + '/cgi-bin/tmp/')

	open(os.getcwd() + '/cgi-bin/tmp/' + os.path.basename(fileitem.filename), 'wb').write(fileitem.file.read())
	page += f"		<h1>The file \"{os.path.basename(fileitem.filename)}\" was uploaded to {os.getcwd()}/cgi-bin/tmp</h1>\n"

except Exception as e:
	page += f"		<h1>Error: {str(e)}</h1>\n"

page += """\
	</body>
</html>"""

print(page)
