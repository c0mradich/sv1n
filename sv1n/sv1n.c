#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "extract.h"
#include "help.h"
#include "client.h"
#include "commands.h"
#include "Server.h"
#include "getIP.h"

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

HANDLE initWindowsConsole() {
		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);

		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		if (hConsole == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "GetStdHandle failed\n");
			return INVALID_HANDLE_VALUE;
		}

		return hConsole;
}

#endif



CRITICAL_SECTION clientsLock;
Client clients[100];
int clientscount = 0;

SOCKET clientSocket = INVALID_SOCKET;
SOCKET* activeSocket = &clientSocket;

int main(int argc, char* argv[]) {
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("WSAStartup failed\n");
		return 1;
	}
	InitializeCriticalSection(&clientsLock);

	HANDLE hConsole = initWindowsConsole();

	if (hConsole == INVALID_HANDLE_VALUE || hConsole == NULL)
	{
		fprintf(stderr, "Error occurred\n");
		return EXIT_FAILURE;
	}

	char* username = NULL;

	char* port = "5000";
	char* host = "127.0.0.1";

	int listenMode = 0;
	int autoconnect = 0;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--help") == 0) {
			showHelp();
			return 0;
		}

		else if (strcmp(argv[i], "--name") == 0) {
			if (i + 1 >= argc){
				fprintf(stderr, "--name requires a value\n");return EXIT_FAILURE;
			}

			if (strlen(argv[i + 1]) > 32) {
				fprintf(stderr, "--name must not exceed 32 characters\n"); return EXIT_FAILURE;
			}

			if (strchr(argv[i + 1], ' ') != NULL) {
				printf("Username contains spaces\n");
				return 0;
			}

			username = argv[i + 1];
		}

		else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--listen") == 0) {
			listenMode = 1;
		}

		else if (strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0) {
			if (i + 1 >= argc)
			{
				fprintf(stderr, "--port requires a value\n");
				return EXIT_FAILURE;
			}
			port = argv[i + 1];
		}
		else if (strcmp(argv[i], "--host") == 0 || strcmp(argv[i], "-h") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "--host requires a value\n");
				return EXIT_FAILURE;
			}
			host = argv[i + 1];
		}
		else if (strcmp(argv[i], "-a") == 0) {
			autoconnect = 1;
		}
	}

		SetConsoleTextAttribute(hConsole, 11); // Blue

		showLogo();

		SetConsoleTextAttribute(hConsole, 7); // Default

		if (listenMode == 1) {
			serverFunc(port, hConsole);
		}
		else {
			if (username == NULL) {
				return EXIT_FAILURE;
			}

			if (autoconnect) {
				char localIP[16];

				if (!getLocalIP(localIP, sizeof(localIP))) {
					printf("[ERROR] Could not determine local IP\n");
					return EXIT_FAILURE;
				}

				char* lastDot = strrchr(localIP, '.');

				if (lastDot == NULL) {
					printf("[ERROR] Invalid local IP\n");
					return EXIT_FAILURE;
				}

				*lastDot = '\0';

				clientFunc(username, localIP, port, hConsole, 1);
			}
			else {
				clientFunc(username, host, port, hConsole, 0);
			}
		}

	SetConsoleCtrlHandler(ConsoleHandler, TRUE);

	WSACleanup();
	DeleteCriticalSection(&clientsLock);
	return 0;
}