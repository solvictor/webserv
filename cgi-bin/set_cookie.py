#! /usr/bin/python3

from http import cookies
import cgi

form = cgi.FieldStorage()

key = form.getvalue("key")
value = form.getvalue("value")

page = "HTTP/1.1 204 OK\n"

if key and value:
	cookie = cookies.SimpleCookie()
	cookie[key] = value
	page += cookie.output()

print(page)
