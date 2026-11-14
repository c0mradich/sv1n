#pragma once
#include <winsock2.h>
#include <windows.h>

DWORD WINAPI clientWorker(LPVOID lpParam);
void serverFunc(char* port, HANDLE hConsole);