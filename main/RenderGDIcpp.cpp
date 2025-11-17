#include "RendererGDI.h"
#include "AppState.h"

extern AppState g_app;

void RendererGDI::Draw(HDC hdc, RECT rc, const ImageManager& img)
{
    if (!img.hasImage) return;

    int drawW = (int)(img.width * g_app.zoom);
    int drawH = (int)(img.height * g_app.zoom);

    int x = g_app.offsetX;
    int y = g_app.offsetY;

    static BITMAPINFO bmi;
    static bool init = false;

    if (!init)
    {
        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biXPelsPerMeter = 0;
        bmi.bmiHeader.biYPelsPerMeter = 0;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;
        init = true;
    }

    bmi.bmiHeader.biWidth = img.width;
    bmi.bmiHeader.biHeight = -img.height;
    bmi.bmiHeader.biSizeImage = 0;

    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, 0);

    StretchDIBits(
        hdc,
        x,
        y,
        drawW,
        drawH,
        0,
        0,
        img.width,
        img.height,
        img.pixels.data(),
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}
