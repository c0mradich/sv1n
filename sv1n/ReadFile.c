#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sendMessage.h"
#include "client.h"

int sendFile(SOCKET socket, const char* filePath, char* username) {

    FILE* file = NULL;
    errno_t err = fopen_s(&file, filePath, "rb");
    if (err != 0 || file == NULL) {
        sendMessage(socket, "[ERROR] FILE NOT FOUND", sizeof("[ERROR] FILE NOT FOUND"));
        return 1;
    }

    int max_size = 64000;

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    char* separator = strrchr(filePath, '/');
    char* separator2 = strrchr(filePath, '\\');

    if (separator2 != NULL && (separator == NULL || separator2 > separator))
        separator = separator2;

    char* filename = separator ? separator + 1 : filePath;

    char initFileTransportMessage[256];

    if (strcmp(username, "SERVER_LISTENER") == 0) {
        sprintf_s(initFileTransportMessage, sizeof(initFileTransportMessage), "SV1N_FILE_SEND1NG %s %ld", filename, fileSize);
    }
    else {
        sprintf_s(initFileTransportMessage, sizeof(initFileTransportMessage), "SV1N_FILE_SEND1NG %s %s %ld", username, filename, fileSize);
    }



    if (sendMessage(socket, initFileTransportMessage, strlen(initFileTransportMessage)) == SOCKET_ERROR) {
        fclose(file);
        return 1;
    }

    int bytes_to_read;

    unsigned char* data = malloc(max_size);

    if (data == NULL) {
        fclose(file);
        return 1;
    }

    while (fileSize != 0) {
        bytes_to_read = (fileSize > max_size) ? max_size : (int)fileSize;

        size_t bytesRead = fread(data, 1, bytes_to_read, file);

        if (bytesRead == 0)
            break;

        if (sendAll(socket, data, (int)bytesRead) == SOCKET_ERROR) {
            free(data);
            fclose(file);
            return 1;
        }

        fileSize -= bytesRead;
    }

    free(data);
    fclose(file);
    return 0;
}

int recvFile(SOCKET socket, FILE* file, long fileSize)
{
    const int bufferSize = 64000;

    unsigned char* buffer = malloc(bufferSize);

    if (buffer == NULL)
        return 1;

    long remaining = fileSize;

    while (remaining > 0)
    {
        int bytesToRecv = remaining > bufferSize ? bufferSize : (int)remaining;

        int bytesReceived = recv(socket, (char*)buffer, bytesToRecv, 0);

        if (bytesReceived <= 0)
        {
            free(buffer);
            fclose(file);
            return 1;
        }

        fwrite(buffer, 1, bytesReceived, file);

        remaining -= bytesReceived;
    }

    free(buffer);
    fclose(file);

    return 0;
}