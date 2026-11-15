
#include <windows.h>
#include <gdiplus.h>
#include <objidl.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

// ============================================================
// SAVE PNG
// ============================================================

extern "C" int saveScreenshotPNG(
    const unsigned char* pngData,
    size_t imageSize
) {
    if (!pngData || imageSize == 0)
        return 0;

    time_t currentTime = time(NULL);

    struct tm timeInfo;

    if (localtime_s(&timeInfo, &currentTime) != 0)
        return 0;

    char filename[128];

    snprintf(
        filename,
        sizeof(filename),
        "screenshot_%04d-%02d-%02d_%02d-%02d-%02d.png",
        timeInfo.tm_year + 1900,
        timeInfo.tm_mon + 1,
        timeInfo.tm_mday,
        timeInfo.tm_hour,
        timeInfo.tm_min,
        timeInfo.tm_sec
    );

    FILE* file = NULL;
    errno_t err = fopen_s(&file, filename, "wb");

    if (err != 0 || !file)
        return 0;

    size_t written = fwrite(
        pngData,
        1,
        imageSize,
        file
    );

    fclose(file);

    if (written != imageSize)
        return 0;


    return 1;
}


// ============================================================
// TAKE SCREENSHOT
// ============================================================

extern "C" unsigned char* takeScreenshotPixels(
    int* width,
    int* height
) {
    if (!width || !height)
        return NULL;

    *width = 0;
    *height = 0;

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    if (w <= 0 || h <= 0)
        return NULL;

    // Check size overflow
    if ((size_t)w > SIZE_MAX / 4 / (size_t)h)
        return NULL;

    size_t imageSize = (size_t)w * h * 4;

    // Get screen DC
    HDC screenDC = GetDC(NULL);

    if (!screenDC)
        return NULL;

    // Create memory DC
    HDC memoryDC = CreateCompatibleDC(screenDC);

    if (!memoryDC) {
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    // Create bitmap
    HBITMAP bitmap = CreateCompatibleBitmap(
        screenDC,
        w,
        h
    );

    if (!bitmap) {
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    // Select bitmap
    HGDIOBJ oldBitmap = SelectObject(
        memoryDC,
        bitmap
    );

    if (!oldBitmap || oldBitmap == HGDI_ERROR) {
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    // Capture screen
    BOOL captured = BitBlt(
        memoryDC,
        0,
        0,
        w,
        h,
        screenDC,
        0,
        0,
        SRCCOPY | CAPTUREBLT
    );

    SelectObject(memoryDC, oldBitmap);

    if (!captured) {
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    // Allocate pixel buffer
    unsigned char* pixels = (unsigned char*)malloc(
        imageSize
    );

    if (!pixels) {
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);
        return NULL;
    }

    // Configure bitmap format
    BITMAPINFO bmi = {};

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    // Get pixel data
    int lines = GetDIBits(
        screenDC,
        bitmap,
        0,
        h,
        pixels,
        &bmi,
        DIB_RGB_COLORS
    );

    // Release GDI resources
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


// ============================================================
// ENCODE SCREENSHOT TO PNG
// ============================================================

extern "C" int encodeScreenshotPNG(
    const unsigned char* pixels,
    int width,
    int height,
    unsigned char** pngData,
    size_t* pngSize
) {
    if (!pixels || !pngData || !pngSize ||
        width <= 0 || height <= 0 || width > INT_MAX / 4) {
        return 0;
    }

    *pngData = NULL;
    *pngSize = 0;

    ULONG_PTR token = 0;
    IStream* stream = NULL;
    HGLOBAL memory = NULL;
    void* lockedData = NULL;
    unsigned char* output = NULL;
    int success = 0;

    do {
        // 1. Initialize GDI+
        Gdiplus::GdiplusStartupInput startupInput;

        if (Gdiplus::GdiplusStartup(
            &token, &startupInput, NULL
        ) != Gdiplus::Ok) {
            break;
        }

        // 2. Create memory stream
        if (CreateStreamOnHGlobal(
            NULL, TRUE, &stream
        ) != S_OK) {
            break;
        }

        // 3. Create bitmap and encode PNG
        {
            Gdiplus::Bitmap bitmap(
                width,
                height,
                width * 4,
                PixelFormat32bppARGB,
                (BYTE*)pixels
            );

            if (bitmap.GetLastStatus() != Gdiplus::Ok) {
                break;
            }

            CLSID encoder;

            if (CLSIDFromString(
                L"{557CF406-1A04-11D3-9A73-0000F81EF32E}",
                &encoder
            ) != S_OK) {
                break;
            }

            Gdiplus::Status status = bitmap.Save(
                stream,
                &encoder,
                NULL
            );

            if (status != Gdiplus::Ok) {
                break;
            }
        } // Bitmap is destroyed here.

        // 4. Get exact stream size
        STATSTG stat;

        if (stream->Stat(&stat, STATFLAG_NONAME) != S_OK) {
            break;
        }

        ULONGLONG exactSize = stat.cbSize.QuadPart;

        if (exactSize == 0 ||
            exactSize > SIZE_MAX ||
            exactSize > INT_MAX) {
            break;
        }

        // 5. Get HGLOBAL
        if (GetHGlobalFromStream(stream, &memory) != S_OK) {
            break;
        }

        // 6. Lock memory
        lockedData = GlobalLock(memory);

        if (lockedData == NULL) {
            break;
        }

        // 7. Allocate output buffer
        output = (unsigned char*)malloc((size_t)exactSize);

        if (output == NULL) {
            break;
        }

        // 8. Copy PNG
        memcpy(output, lockedData, (size_t)exactSize);

        *pngData = output;
        *pngSize = (size_t)exactSize;

        output = NULL;
        success = 1;

    } while (0);

    // Cleanup
    if (lockedData != NULL) {
        GlobalUnlock(memory);
    }

    if (output != NULL) {
        free(output);
    }

    if (stream != NULL) {
        stream->Release();
    }

    if (token != 0) {
        Gdiplus::GdiplusShutdown(token);
    }

    return success;
}
