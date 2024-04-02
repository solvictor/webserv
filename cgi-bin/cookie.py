from http import cookies
import os

cookie = cookies.BaseCookie()

print("Content-type: text/html\r\n")
if 'HTTP_COOKIE' in os.environ: 
	cookie.load(os.environ["HTTP_COOKIE"])
	# cookie.clear()
	# print(cookie.output(), '<br>')
	for a, morsel in cookie.items():
		print(a, ':', morsel.value, '<br>')
else:
	print("No Cookies Set Yet!")