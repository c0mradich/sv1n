#pragma once
#include <windows.h>

#ifndef EXTRACT_H
#define EXTRACT_H

char* extractBuffer(char* buffer);
char* messageForm(void);
char* extractFirstPart(char* buffer);
char* execCommand(const char* command);
DWORD startsp(char* buffer, DWORD* err, DWORD* pid);

#endif