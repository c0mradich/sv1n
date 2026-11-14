#pragma once

#ifndef EXTRACT_H
#define EXTRACT_H

char* extractBuffer(char* buffer);
char* messageForm(void);
char* extractFirstPart(char* buffer);
char* execCommand(const char* command);
char* defName(char* buffer);
#endif