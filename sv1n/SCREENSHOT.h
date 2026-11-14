#pragma once
#include <stdint.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bitsPerPixel;
    uint32_t dataLength;
} ScreenshotHeader;

unsigned char* takeScreenshotPixels(int* width, int* height);