#pragma once

#include <winsock2.h>
#include <stddef.h>

int sendMessage(SOCKET socket, const char* message, size_t length);