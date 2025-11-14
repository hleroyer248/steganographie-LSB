#include "RendererGDI.h"

void RendererGDI::Draw(HDC hdc, RECT rc, const ImageManager& img)
{
    if (!img.hasImage) return;

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = img.width;
    bmi.bmiHeader.biHeight = -img.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    StretchDIBits(
        hdc,
        0, 0, w, h,
        0, 0, img.width, img.height,
        img.pixels.data(),
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}
