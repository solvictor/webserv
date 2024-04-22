#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CONNECTION_TIMEOUT 10
#define MESSAGE_BUFFER 40000
#define MAX_URI_LENGTH 4096
#define MAX_CONTENT_LENGTH 30000000