#pragma once

#include <winsock2.h>
#include <stdio.h>

int recvFile(SOCKET socket, FILE* file, long fileSize);
int sendFile(SOCKET socket, const char* filePath, char* username);