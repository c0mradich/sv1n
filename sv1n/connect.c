#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

int connectTimeout(SOCKET socket, const struct sockaddr* address, int addressLength, int timeoutMs)
{
    u_long mode = 1;

    if (ioctlsocket(socket, FIONBIO, &mode) == SOCKET_ERROR)
        return SOCKET_ERROR;

    int result = connect(socket, address, addressLength);

    if (result == 0) {
        mode = 0;
        ioctlsocket(socket, FIONBIO, &mode);
        return 0;
    }

    int error = WSAGetLastError();

    if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS) {
        mode = 0;
        ioctlsocket(socket, FIONBIO, &mode);
        return SOCKET_ERROR;
    }

    fd_set writeSet;

    FD_ZERO(&writeSet);
    FD_SET(socket, &writeSet);

    struct timeval timeout;

    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    result = select(
        0,
        NULL,
        &writeSet,
        NULL,
        &timeout
    );

    if (result <= 0) {
        mode = 0;
        ioctlsocket(socket, FIONBIO, &mode);
        return SOCKET_ERROR;
    }

    int socketError = 0;
    int errorLength = sizeof(socketError);

    getsockopt(
        socket,
        SOL_SOCKET,
        SO_ERROR,
        (char*)&socketError,
        &errorLength
    );

    mode = 0;
    ioctlsocket(socket, FIONBIO, &mode);

    if (socketError != 0)
        return SOCKET_ERROR;

    return 0;
}

SOCKET autoConnect(char* networkPrefix, char* port)
{
    for (int i = 1; i <= 255; i++) {

        char ip[16];

        snprintf(ip, sizeof(ip), "%s.%d", networkPrefix ,i);


        struct addrinfo hints;
        struct addrinfo* result = NULL;

        memset(&hints, 0, sizeof(hints));

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        int status = getaddrinfo(
            ip,
            port,
            &hints,
            &result
        );

        if (status != 0)
            continue;

        SOCKET socketHandle = socket(
            result->ai_family,
            result->ai_socktype,
            result->ai_protocol
        );

        if (socketHandle == INVALID_SOCKET) {
            freeaddrinfo(result);
            continue;
        }

        if (connectTimeout(
            socketHandle,
            result->ai_addr,
            (int)result->ai_addrlen,
            100
        ) == 0) {

            printf("[+] Server found: %s:%s\n", ip, port);

            freeaddrinfo(result);

            return socketHandle;
        }

        closesocket(socketHandle);
        freeaddrinfo(result);
    }

    return INVALID_SOCKET;
}