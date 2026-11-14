#include <windows.h>
#include <stdio.h>


void logError(const char* error, HANDLE hConsole) {
    SetConsoleTextAttribute(hConsole, 12);
    printf("\n%s\n", error);
    SetConsoleTextAttribute(hConsole, 7);
}

void logSuccess(const char* success, HANDLE hConsole) {
    SetConsoleTextAttribute(hConsole, 2);
    printf("\n%s\n", success);
    SetConsoleTextAttribute(hConsole, 7);
}

void logInfo(const char* info, HANDLE hConsole) {
    SetConsoleTextAttribute(hConsole, 14);
    printf("\n%s\n", info);
    SetConsoleTextAttribute(hConsole, 7);
}

void resetWindowsConsoleColor(HANDLE hConsole) {
    SetConsoleTextAttribute(hConsole, 7);
}