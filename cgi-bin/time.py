#! /usr/bin/python3

from datetime import datetime

timestamp = datetime.strftime(datetime.now(), "%H:%M:%S")

while 1:
	pass

page = f"""\
HTTP/1.1 200 OK
Content-type: text/html\r

<!DOCTYPE html>
<html lang="en">
	<head>
		<title>Current time</title>
	</head>
	<body>
		<h1 style="text-align: center;font-size: 1000%;">{timestamp}</h1>
	</body>
</html>
"""

print(page)