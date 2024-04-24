#! /usr/bin/python3

import os
import cgi
import time
import hashlib
import pickle
import sys
from http import cookies
from logger import *
from collections import defaultdict

SESSION_PATH = "cgi-bin/sessions"

if not os.path.exists(SESSION_PATH):
	os.umask(0)
	os.mkdir(SESSION_PATH)

class Session:
	def __init__(self, name):
		self.name = name
		self.sid = hashlib.sha1(str(time.time()).encode("utf-8")).hexdigest()

		with open(SESSION_PATH + "/session_" + self.sid, "wb") as f:
			pickle.dump(self, f)


# Stores Users and their data
class UserDataBase:
	def __init__(self):
		self.user_pass = defaultdict(str)
		self.user_firstname = defaultdict(str)

	def addUser(self, username, password, firstname):
		self.user_pass[username] = password
		self.user_firstname[username] = firstname
		with open("cgi-bin/user_database", "wb") as f:
			pickle.dump(self, f)


def printAccPage(session):
	print(f"""Content-Type: text/html\r\n
<html>
	<head>
		<title>Account Page</title>
	</head>
	<body>
		<h1>Welcome again {session.name}!</h1>
		<p>Your Session ID is: {session.sid}</p>
	</body>
	<a href="/index.html"> Click here to go back to homepage </a>
</html>""")


def printUserMsg(msg):
	print(f"""Content-Type: text/html\r\n
<html>
	<head>
		<title>USER MSG</title>
	</head>
	<body>
		<h1>{msg}</h1>
	</body>
	<a href="/login.html"> Click here to go back to login page </a>
</html>""")


def printLogin():
	print("""Content-Type: text/html\r\n
<html>
	<head>
		<meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
		<link rel="stylesheet" href="/assets/css/accstyle.css">
		<title> Login Page </title>
	</head>
	<body>
		<center><h1> webserv Login form </h1></center>
		<form action = "../cgi-bin/acc.py" method = "get">
			<div class="container">
				<label>Username :</label>
				<input type="text" placeholder="Enter username" name="username" required>
				<label>Password :</label>
				<input type="password" placeholder="Enter password" name="password" required>
				<button type="submit">Login</button>
				No Account? <a href="/register.html">Register Here </a>
			</div>
		</form>
	</body>
</html>""")


def authUser(name, password):
	if not os.path.exists("cgi-bin/user_database"):
		return None
	with open("cgi-bin/user_database", "rb") as f:
		database = pickle.load(f)
		if database.user_pass[name] == password:
			return Session(database.user_firstname[name])
	return None


def handleLogin(form, env_cookies):
	username = form.getvalue("username")
	password = form.getvalue("password")
	firstname = form.getvalue("firstname")
	if username is None:
		printLogin()
	elif firstname is None:
		session = authUser(form.getvalue("username"), form.getvalue("password"))
		if session is None:
			printUserMsg("Failed to login, username or password is wrong!")
		elif env_cookies is not None:
			env_cookies.clear()
			env_cookies["SID"] = session.sid
			env_cookies["SID"]["expires"] = 120  # Session Expires after 2 mins
			print("HTTP/1.1 301 OK")
			print(env_cookies.output())
			print("location: acc.py")
			print("\r\n")
	else:
		if os.path.exists("cgi-bin/user_database"):
			with open("cgi-bin/user_database", "rb") as f:
				database = pickle.load(f)
				if database.user_pass[username]:
					printUserMsg("Username is already registered !")
				else:
					database.addUser(username, password, firstname)
					printUserMsg("Account registered successfully!")
		else:
			database = UserDataBase()
			if database.user_pass[username]:
				printUserMsg("Username is already registered !")
			else:
				database.addUser(username, password, firstname)
				printUserMsg("Account registered successfully!")


form = cgi.FieldStorage()

if "HTTP_COOKIE" in os.environ:
	env_cookies = cookies.SimpleCookie()
	env_cookies.load(os.environ["HTTP_COOKIE"])

	if "SID" in env_cookies:
		log("Your Session ID is", env_cookies["SID"].value, color=BLUE, file=sys.stderr)
		with open(SESSION_PATH + "/session_" + env_cookies["SID"].value, "rb") as f:
			sess = pickle.load(f)
			printAccPage(sess)
	else:
		handleLogin(form, env_cookies)
else:
	handleLogin(form, None)
