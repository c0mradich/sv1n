#pragma once
#include <windows.h>

void resetWindowsConsoleColor(HANDLE hConsole);
void logError(char* error, HANDLE hConsole);
void logSuccess(char* success, HANDLE hConsole);
void logInfo(const char* info, HANDLE hConsole);