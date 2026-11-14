
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

unsigned char* takeScreenshotPixels(int* width, int* height)
{
    *width = 0;
    *height = 0;

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    if (w <= 0 || h <= 0)
        return NULL;

    // Check for size_t overflow.
    if ((size_t)w > SIZE_MAX / 4 / (size_t)h)
        return NULL;

    size_t imageSize = (size_t)w * h * 4;

    HDC screenDC = GetDC(NULL);

    if (!screenDC)
        return NULL;

    HDC memoryDC = CreateCompatibleDC(screenDC);

    if (!memoryDC) {
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    HBITMAP bitmap = CreateCompatibleBitmap(
        screenDC, w, h
    );

    if (!bitmap) {
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    HGDIOBJ oldBitmap = SelectObject(memoryDC, bitmap);

    if (!oldBitmap || oldBitmap == HGDI_ERROR) {
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    BOOL captured = BitBlt(
        memoryDC,
        0, 0,
        w, h,
        screenDC,
        0, 0,
        SRCCOPY | CAPTUREBLT
    );

    SelectObject(memoryDC, oldBitmap);

    if (!captured) {
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    unsigned char* pixels = malloc(imageSize);

    if (!pixels) {
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    BITMAPINFO bmi = { 0 };

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    int lines = GetDIBits(
        screenDC,
        bitmap,
        0,
        h,
        pixels,
        &bmi,
        DIB_RGB_COLORS
    );

    DeleteObject(bitmap);
    DeleteDC(memoryDC);
    ReleaseDC(NULL, screenDC);

    if (lines != h) {
        free(pixels);
        return NULL;
    }

    *width = w;
    *height = h;

    return pixels;
}
