#pragma once

#include <fcntl.h>

#include <sys/wait.h>

#include <arpa/inet.h>

#define CONNECTION_TIMEOUT 60
#define MESSAGE_BUFFER 40000
#define MAX_URI_LENGTH 4096
#define MAX_CONTENT_LENGTH 30000000