#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

int getLocalIP(char* ip, size_t ipSize)
{
    char hostname[256];

    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
        return 0;

    struct addrinfo hints;
    struct addrinfo* result = NULL;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0)
        return 0;

    int success = 0;

    for (struct addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        struct sockaddr_in* address = (struct sockaddr_in*)ptr->ai_addr;

        char* foundIP = inet_ntoa(address->sin_addr);

        if (foundIP != NULL &&
            strncmp(foundIP, "127.", 4) != 0) {

            snprintf(ip, ipSize, "%s", foundIP);
            success = 1;
            break;
        }
    }

    freeaddrinfo(result);
    return success;
}