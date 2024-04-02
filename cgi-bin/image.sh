#!/bin/bash

echo -e 'HTTP/1.1 200 OK'
echo -e 'Content-Type: image/jpeg\r\n'

# paste the full path to any image below
cat ./ressources/avatar.png

