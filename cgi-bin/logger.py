from datetime import datetime

RESET = "\033[0m"
BOLD = "\033"
RED = "\033[31m"
LIGHT_RED = "\033[1;31m"
GREEN = "\033[32m"
LIGHT_GREEN = "\033[1;32m"
YELLOW = "\033[33m"
LIGHT_YELLOW = "\033[1;33m"
BLUE = "\033[34m"
LIGHT_BLUE = "\033[1;34m"

def log(*args, **kwargs):
	now = datetime.now()
	timestamp = now.strftime("%d-%m-%Y  %H:%M:%S")
	color = ""
	if "color" in kwargs:
		color = kwargs["color"]
		del kwargs["color"]
	print(f"{color}[{timestamp}] ", *args, RESET if color else "", **kwargs)
