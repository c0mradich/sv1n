#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <string.h>
#include "SCREENSHOT.h"
#include "extract.h"
#include "client.h"
#include "sendMessage.h"
#include "commands.h"
#include "ReadFile.h"
#include "connect.h"
#include "Log.h"


extern Client clients[100];
extern int clientscount;
extern CRITICAL_SECTION clientsLock;
extern SOCKET* activeSocket;


int registerClient(
    SOCKET clientSocket,
    char** usernameOut,
	HANDLE hConsole
) {
    uint32_t networkLength;

    *usernameOut = NULL;

    // Receive message length
    int result = recvAll(
        clientSocket,
        (char*)&networkLength,
        sizeof(networkLength)
    );

    if (result != sizeof(networkLength)) {
        return 0;
    }

    uint32_t messageLength = ntohl(networkLength);

    if (messageLength == 0 || messageLength >= 1024) {
        return 0;
    }

    // Receive message payload
    char buffer[1024];

    result = recvAll(
        clientSocket,
        buffer,
        (int)messageLength
    );

    if (result != (int)messageLength) {
        return 0;
    }

    buffer[messageLength] = '\0';

    // Parse registration command
    char* splitter = strchr(buffer, ' ');

    if (splitter == NULL) {
        return 0;
    }

    int firstLength = (int)(splitter - buffer);

    char primary[64];

    if (firstLength >= sizeof(primary)) {
        return 0;
    }

    memcpy(primary, buffer, firstLength);
    primary[firstLength] = '\0';

    if (strcmp(primary, "SV1N_REGISTERUSER") != 0) {
        return 0;
    }

    char* usernameStart = splitter + 1;
    size_t usernameLen = strlen(usernameStart);

    if (usernameLen == 0 ||
        usernameLen >= sizeof(clients[0].username)) {
        return 0;
    }

    char* username = malloc(usernameLen + 1);

    if (username == NULL) {
        return 0;
    }

    memcpy(username, usernameStart, usernameLen + 1);

    // Check username and register client
    EnterCriticalSection(&clientsLock);

    for (int user = 0; user < clientscount; user++) {
        if (strcmp(clients[user].username, username) == 0) {
            LeaveCriticalSection(&clientsLock);

            const char* response = "USERNAME_TAKEN";
            uint32_t responseLength = (uint32_t)strlen(response);
            uint32_t networkResponseLength = htonl(responseLength);

			sendMessage(
				clientSocket,
				response,
				responseLength
			);

            free(username);
            return 0;
        }
    }

    if (clientscount >= 100) {
        LeaveCriticalSection(&clientsLock);
        free(username);
        return 0;
    }

    Client* client = &clients[clientscount];

    client->socket = clientSocket;

    strcpy_s(
        client->username,
        sizeof(client->username),
        username
    );

    clientscount++;

    LeaveCriticalSection(&clientsLock);

    *usernameOut = username;

	char message[128];

	snprintf(message, sizeof(message), "[+] Client successfully registered: %s\n", client->username);

	logSuccess(message, hConsole);

    return 1;
}

void removeClient(const char* username)
{
	EnterCriticalSection(&clientsLock);

	for (int i = 0; i < clientscount; i++) {
		if (strcmp(clients[i].username, username) == 0) {
			clients[i] = clients[clientscount - 1];
			clientscount--;
			break;
		}
	}

	LeaveCriticalSection(&clientsLock);
}


void sendClientList(SOCKET clientSocket)
{
	char response[4096] = "[SUCCESS] CLIENTLIST";

	EnterCriticalSection(&clientsLock);

	for (int i = 0; i < clientscount; i++) {
		strcat_s(response, sizeof(response), " ");
		strcat_s(response, sizeof(response), clients[i].username);
	}

	LeaveCriticalSection(&clientsLock);

	sendMessage(clientSocket, response, strlen(response));
}

void broadcastMessage(SOCKET sender, const char* message, int len) {
	printf("%s\n", message);
	EnterCriticalSection(&clientsLock);

	for (int i = 0; i < clientscount; i++) {
		if (clients[i].socket != sender)
			sendMessage(clients[i].socket, message, len);
	}

	LeaveCriticalSection(&clientsLock);
}

int recvAll(SOCKET socket, char* buffer, int length)
{
    int totalReceived = 0;


    while (totalReceived < length) {

        int received = recv(
            socket,
            buffer + totalReceived,
            length - totalReceived,
            0
        );

        if (received == 0) {
                        return 0; // disconnected
        }

        if (received == SOCKET_ERROR) {
            int err = WSAGetLastError();
                        return SOCKET_ERROR;
        }

        totalReceived += received;
            }

        return totalReceived;
}

int sendAll(SOCKET socket, const void* buffer, int length)
{
	int totalSent = 0;
	const char* ptr = (const char*)buffer;


	while (totalSent < length)
	{
		int bytesSent = send(
			socket,
			ptr + totalSent,
			length - totalSent,
			0
		);

		if (bytesSent <= 0) {
			int err = WSAGetLastError();
						return SOCKET_ERROR;
		}

		totalSent += bytesSent;
			}

		return totalSent;
}

//CLIENT

//Client Receive Worker

BOOL WINAPI ConsoleHandler(DWORD signal)
{
	switch (signal)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		if (activeSocket != NULL && *activeSocket != INVALID_SOCKET)
			shutdown(*activeSocket, SD_BOTH);
		return FALSE;
	}
	return FALSE;
}

DWORD WINAPI receiverWorker(LPVOID lpParam){
	ClientReceiverData* data = (ClientReceiverData*)lpParam;

	SOCKET clientSocket = data->socket;
	HANDLE hConsole = data->hConsole;
	char* username = data->username;

	free(data);

	// Registration
	size_t usernameLen = strlen(username);
	size_t registerLength = strlen("SV1N_REGISTERUSER") + 1 + usernameLen;

	char* registerMSG = malloc(registerLength + 1);

	if (registerMSG == NULL) {
		return 0;
	}

	snprintf(
		registerMSG,
		registerLength + 1,
		"%s %s",
		"SV1N_REGISTERUSER",
		username
	);

	uint32_t networkLength = htonl((uint32_t)registerLength);

	// Send registration header
	int result = sendMessage(
		clientSocket,
		registerMSG,
		registerLength
	);

	free(registerMSG);

	if (result == SOCKET_ERROR) {
		return 0;
	}

	// Receive messages
	while (1) {

		uint32_t networkMessageLength;


		result = recvAll(
			clientSocket,
			(char*)&networkMessageLength,
			sizeof(networkMessageLength)
		);


		if (result == 0 || result == SOCKET_ERROR) {
			logError("[ERROR] Failed to receive message length", hConsole);
						break;
		}

		uint32_t messageLength = ntohl(networkMessageLength);


		if (messageLength == 0 || messageLength > MAX_MESSAGE_SIZE) {
			logError("[ERROR] Invalid message length", hConsole);
						break;
		}

		char* buffer = malloc((size_t)messageLength + 1);

		if (buffer == NULL) {
			logError("[ERROR] Failed to allocate message buffer", hConsole);
			break;
		}


		result = recvAll(
			clientSocket,
			buffer,
			(int)messageLength
		);


		if (result == 0 || result == SOCKET_ERROR) {
			free(buffer);
			logError("[ERROR] Disconnected from server", hConsole);
						break;
		}

		buffer[messageLength] = '\0';


		if (strcmp(buffer, "USERNAME_TAKEN") == 0) {
			logError("[ERROR] This name is already taken.", hConsole);
			free(buffer);
			closesocket(clientSocket);
			exit(0);
		}


		if (clientHandleFunc(clientSocket, username, buffer, hConsole)) {
			free(buffer);
			continue;
		}

		// Check if this looks like PNG data (starts with PNG signature)
		if (messageLength >= 4 &&
			(unsigned char)buffer[0] == 0x89 &&
			(unsigned char)buffer[1] == 0x50 &&
			(unsigned char)buffer[2] == 0x4E &&
			(unsigned char)buffer[3] == 0x47) {
			(void)saveScreenshotPNG((unsigned char*)buffer, (size_t)messageLength);
			free(buffer);
			continue;
		}

		char* firstPart = extractFirstPart(buffer);

		if (firstPart != NULL) {

			if (strcmp(firstPart, "[SUCCESS]") == 0) {
				logSuccess(buffer, hConsole);
			}
			else if (strcmp(firstPart, "[ERROR]") == 0) {
				logError(buffer, hConsole);
			}
			else {
				printf("\n%s\n", buffer);
			}
			free(firstPart);
		}
		free(buffer);
	}
	return 0;
}


void clientFunc(char* username, char* host, char* port, HANDLE hConsole, int autoconnect) {

	SOCKET clientSocket = INVALID_SOCKET;

	if (autoconnect) {

		clientSocket = autoConnect(host, port);

		if (clientSocket == INVALID_SOCKET) {
			logError("[ERROR] Server not found", hConsole);
			return;
		}

	}
	else {

		struct addrinfo hints;
		struct addrinfo* result = NULL;

		memset(&hints, 0, sizeof(hints));

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		int status = getaddrinfo(host, port, &hints, &result);

		if (status != 0) {
			logError("[ERROR] getaddrinfo failed", hConsole);
			return;
		}

		clientSocket = socket(
			result->ai_family,
			result->ai_socktype,
			result->ai_protocol
		);

		if (clientSocket == INVALID_SOCKET) {
			logError("[ERROR] Socket failed.", hConsole);
			freeaddrinfo(result);
			return;
		}

		if (connect(
			clientSocket,
			result->ai_addr,
			(int)result->ai_addrlen
		) == SOCKET_ERROR) {

			logError("[ERROR] Connect failed.", hConsole);

			closesocket(clientSocket);
			freeaddrinfo(result);

			return;
		}

		freeaddrinfo(result);
	}

	logSuccess("[+] Connected!", hConsole);

	*activeSocket = clientSocket;

	ClientReceiverData* data = malloc(sizeof(ClientReceiverData));

	if (data == NULL) {
		logError("[ERROR] Failed to allocate receiver data", hConsole);
		closesocket(clientSocket);
		return;
	}

	data->socket = clientSocket;
	data->username = username;
	data->hConsole = hConsole;

	HANDLE hThread = CreateThread(NULL, 0, receiverWorker, data, 0, NULL);

	if (hThread == NULL) {
		logError("[ERROR] Failed to create receiver thread", hConsole);

		free(data);
		closesocket(clientSocket);

		return;
	}

	//CloseHandle(hThread);

	int usernameLen = strlen(username);

	while (1) {

		Sleep(50);

		printf("%s> ", username);

		char* message = messageForm();

		if (message == NULL) {
			logError("[ERROR] Failed to allocate message buffer", hConsole);
			break;
		}

		if (strlen(message) > 0) {

			if (strcmp(message, "exit") == 0) {
				free(message);
				break;
			}

			if (strcmp(message, "cls") == 0) {
				system("cls");
				free(message);
				continue;
			}

			if (strncmp(message, "dataPush", 8) == 0) {
				char* command = extractBuffer(message);

				if (command == NULL) {
					free(message);
					continue;
				}

				char* opp = extractFirstPart(command);
				char* filename = extractBuffer(command);

				if (opp == NULL || filename == NULL) {
					free(command);
					free(opp);
					free(filename);
					free(message);
					continue;
				}

				int res = sendFile(clientSocket, filename, opp);

				if (res) {
					logError("[ERROR] FAILED TO SEND A FILE ", hConsole);
				}

				free(command);
				free(opp);
				free(filename);
				free(message);
				continue;
			}

			size_t bufferSize =
				usernameLen + strlen(message) + 3;

			char* buffer = malloc(bufferSize);

			if (buffer != NULL) {

				snprintf(
					buffer,
					bufferSize,
					"%s> %s",
					username,
					message
				);

				size_t messageLength = strlen(buffer);

				int result = sendMessage(
					clientSocket,
					buffer,
					messageLength
				);

				if (result == SOCKET_ERROR) {
					printf("[ERROR] Failed to send message");
				}

				free(buffer);
			}
		}

		free(message);
	}
	shutdown(clientSocket, SD_BOTH);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	closesocket(clientSocket);
	if (activeSocket != NULL && *activeSocket == clientSocket)
		*activeSocket = INVALID_SOCKET;
}

Client* findUser(char* username)
{
	if (username == NULL)
		return NULL;

	EnterCriticalSection(&clientsLock);

	for (int i = 0; i < clientscount; i++) {
		if (strcmp(clients[i].username, username) == 0) {
			Client* found = &clients[i];

			LeaveCriticalSection(&clientsLock);
			return found;
		}
	}

	LeaveCriticalSection(&clientsLock);
	return NULL;
}