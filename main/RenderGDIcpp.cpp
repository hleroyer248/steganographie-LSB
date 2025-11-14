#include "RendererGDI.h"
#include "AppState.h"

extern AppState g_app;

void RendererGDI::Draw(HDC hdc, RECT rc, const ImageManager& img)
{
    if (!img.hasImage) return;

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    int drawW = (int)(img.width * g_app.zoom);
    int drawH = (int)(img.height * g_app.zoom);

    int x = g_app.offsetX;
    int y = g_app.offsetY;

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = img.width;
    bmi.bmiHeader.biHeight = -img.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, 0);

    StretchDIBits(
        hdc,
        x, y, drawW, drawH,
        0, 0, img.width, img.height,
        img.pixels.data(),
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}
