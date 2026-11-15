#pragma once
#include <winsock2.h>

int handleCommands(SOCKET clientSocket, char* username, char* buffer);
int clientHandleFunc(SOCKET clientSocket, char* username, char* buffer, HANDLE hConsole);
int SERVER_RETRANSLATOR(char* buffer, SOCKET clientSocket);