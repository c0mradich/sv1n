#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        uint32_t width;
        uint32_t height;
        uint32_t dataLength;
    } ScreenshotHeader;

    unsigned char* takeScreenshotPixels(int* width, int* height);

    int encodeScreenshotPNG(
        const unsigned char* pixels,
        int width,
        int height,
        unsigned char** pngData,
        size_t* pngSize
    );

    int saveScreenshotPNG(
        const unsigned char* pngData,
        size_t imageSize
    );

#ifdef __cplusplus
}
#endif