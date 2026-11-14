#include <winsock2.h>
#include <stdint.h>
#include "client.h"

int sendMessage(SOCKET socket, const char* message, size_t length)
{
    if (message == NULL || length == 0 || length > MAX_MESSAGE_SIZE)
        return SOCKET_ERROR;

    uint32_t networkLength = htonl((uint32_t)length);

    int result = sendAll(
        socket,
        (const char*)&networkLength,
        sizeof(networkLength)
    );

    if (result != sizeof(networkLength))
        return SOCKET_ERROR;

    result = sendAll(socket, message, (int)length);

    if (result != (int)length)
        return SOCKET_ERROR;

    return result;
}