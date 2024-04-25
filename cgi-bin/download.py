#! /usr/bin/python3

import cgi
import os
import sys
import mimetypes
from logger import *


# Function to read a file and return its content
def get_file_content(filepath):
    with open(filepath, "rb") as f:
        return f.read()


# Get the filename from the query parameters
form = cgi.FieldStorage()
filename = form.getvalue("filename")

# Set the path to the directory where uploaded files are stored
upload_dir = os.getcwd() + "/cgi-bin/tmp/"

# Get the MIME type based on the file extension
mime_type, _ = mimetypes.guess_type(filename)

# Set the content type to force the browser to download the file
if mime_type is None:
    mime_type = "application/octet-stream"  # Default to binary if MIME type is unknown

log(mime_type, file=sys.stderr)

print('Content-Disposition: attachment; filename="' + filename + '"')
print("Content-Type: " + mime_type + "\r\n")

try:
    # Open and read the file
    file_content = get_file_content(upload_dir + filename)
    # Print the file content to the standard output (this will be sent to the browser)
    print(file_content)
except FileNotFoundError:
    print("File not found.")
