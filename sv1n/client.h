#pragma once
#include <winsock2.h>
#include <windows.h>

#define MAX_MESSAGE_SIZE (10 * 1024 * 1024)

typedef struct {
	SOCKET socket;
	char username[32];
} Client;

typedef struct {
	SOCKET clientSocket;
	HANDLE hConsole;
} WorkerData;

typedef struct {
	SOCKET socket;
	char* username;
	HANDLE hConsole;
}ClientReceiverData;

typedef struct {
	DWORD signal;
	SOCKET clientSocket;
}HandlerWorker;

void sendClientList(SOCKET clientSocket);
void broadcastMessage(SOCKET sender, const char* message, int len);

int recvAll(SOCKET socket, char* buffer, int length);
int sendAll(SOCKET socket, const void* buffer, int length);

void removeClient(const char* username);
int registerClient(SOCKET clientSocket, char** usernameOut, HANDLE hConsole);

DWORD WINAPI receiverWorker(LPVOID lpParam);
BOOL WINAPI ConsoleHandler(DWORD signal);

void clientFunc(char* username, char* host, char* port, HANDLE hConsole, int autoconnect);
Client* findUser(char* username);
