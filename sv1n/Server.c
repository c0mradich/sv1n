#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "client.h"
#include "extract.h"
#include "commands.h"

//Server Worker.

extern Client clients[100];
extern int clientscount;
extern CRITICAL_SECTION clientsLock;
extern SOCKET* activeSocket;

DWORD WINAPI clientWorker(LPVOID lpParam)
{
	WorkerData* data = (WorkerData*)lpParam;
	SOCKET clientSocket = data->clientSocket;

	HANDLE hConsole = data->hConsole;

	free(data);

	char* username = NULL;

	if (!registerClient(clientSocket, &username, hConsole)) {
		closesocket(clientSocket);
		return 0;
	}

	while (1) {
		uint32_t networkLength;

		int result = recvAll(clientSocket, (char*)&networkLength, sizeof(networkLength));

		if (result == 0 || result == SOCKET_ERROR) {
			removeClient(username);
			break;
		}


		uint32_t messageLength = ntohl(networkLength);


		if (messageLength == 0 || messageLength > MAX_MESSAGE_SIZE) {
			printf("Invalid message length\n");
			break;
		}

		char* buffer = malloc(messageLength + 1);

		if (buffer == NULL) {
			// allocation error
			break;
		}

		result = recvAll(
			clientSocket,
			buffer,
			messageLength
		);

		if (result == 0 || result == SOCKET_ERROR) {
			removeClient(username);
			free(buffer);
			break;
		}

		buffer[messageLength] = '\0';

		printf("\nBUFFER: %s\n", buffer);

		char* extractedBuffer = extractBuffer(buffer);

		if (extractedBuffer != NULL && strcmp(extractedBuffer, "GETCLIENTS") == 0) {
			sendClientList(clientSocket);
			free(extractedBuffer);
			free(buffer);
			continue;
		}

		if (SERVER_RETRANSLATOR(buffer, clientSocket) == 1) {
			free(buffer);
			continue;
		}

		if (handleCommands(clientSocket, username, buffer))
		{
			free(buffer);
			continue;
		}

		broadcastMessage(clientSocket, buffer, strlen(buffer));
		free(buffer);
	}

	closesocket(clientSocket);

	return 0;
}


//Start Server

void serverFunc(char* port, HANDLE hConsole) {
	struct addrinfo hints;
	struct addrinfo* result = NULL;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int status = getaddrinfo(NULL, port, &hints, &result);

	if (status != 0) {
		printf("getaddrinfo failed\n");
		return;
	}

	SOCKET serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (serverSocket == INVALID_SOCKET) {
		printf("socket failed\n");
		freeaddrinfo(result);
		return;
	}

	if (bind(serverSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		printf("bind failed\n");
		closesocket(serverSocket);
		freeaddrinfo(result);
		return;
	}

	freeaddrinfo(result);

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("listen failed\n");
		closesocket(serverSocket);
		return;
	}

	*activeSocket = serverSocket;

	printf("%s %s\n", "[*] Listening on port", port);
	printf("[*] Waiting for clients...\n");

	while (1) {
		SOCKET clientSocket = accept(serverSocket, NULL, NULL);

		if (clientSocket == INVALID_SOCKET) {
			printf("accept failed\n");
			closesocket(serverSocket);
			return;
		}

		printf("[+] Client connected!\n");

		WorkerData* data = malloc(sizeof(WorkerData));
		if (data == NULL) {
			closesocket(clientSocket);
			continue;
		}

		data->clientSocket = clientSocket;
		data->hConsole = hConsole;

		HANDLE hThread = CreateThread(NULL, 0, clientWorker, data, 0, NULL);

		if (hThread == NULL) { free(data); closesocket(clientSocket); continue; }

		CloseHandle(hThread);
	}
	closesocket(serverSocket);
}