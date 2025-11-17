#include "ImageManager.h"
#include <windows.h>

ImageManager::ImageManager() : width(0), height(0), hasImage(false)
{
}

bool ImageManager::LoadBMP(const char* path)
{
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE) return false;

    DWORD br = 0;
    BITMAPFILEHEADER bmf;
    if (!ReadFile(f, &bmf, sizeof(bmf), &br, 0) || br != sizeof(bmf))
    {
        CloseHandle(f);
        return false;
    }
    if (bmf.bfType != 0x4D42)
    {
        CloseHandle(f);
        return false;
    }

    BITMAPINFOHEADER bih;
    if (!ReadFile(f, &bih, sizeof(bih), &br, 0) || br != sizeof(bih))
    {
        CloseHandle(f);
        return false;
    }

    if (bih.biCompression != BI_RGB)
    {
        CloseHandle(f);
        return false;
    }
    if (bih.biBitCount != 24 && bih.biBitCount != 32)
    {
        CloseHandle(f);
        return false;
    }

    int w = bih.biWidth;
    int h = bih.biHeight > 0 ? bih.biHeight : -bih.biHeight;

    if (w <= 0 || h <= 0)
    {
        CloseHandle(f);
        return false;
    }

    width = w;
    height = h;

    SetFilePointer(f, bmf.bfOffBits, 0, FILE_BEGIN);

    int bpp = bih.biBitCount / 8;
    int rowRaw = width * bpp;
    int pad = (bpp == 3) ? ((4 - (rowRaw % 4)) & 3) : 0;
    int stride = rowRaw + pad;

    const int totalSize = stride * height;
    std::vector<unsigned char> src;
    src.resize(totalSize);

    if (!ReadFile(f, src.data(), (DWORD)totalSize, &br, 0) || br != (DWORD)totalSize)
    {
        CloseHandle(f);
        return false;
    }

    CloseHandle(f);

    const int pixCount = width * height;
    pixels.clear();
    pixels.resize(pixCount * 4);

    const bool bottom = bih.biHeight > 0;

    for (int y = 0; y < height; ++y)
    {
        int sy = bottom ? (height - 1 - y) : y;
        const unsigned char* row = src.data() + sy * stride;
        int dstIndex = y * width * 4;

        if (bpp == 3)
        {
            for (int x = 0; x < width; ++x)
            {
                const int s = x * 3;
                pixels[dstIndex + 0] = row[s + 0];
                pixels[dstIndex + 1] = row[s + 1];
                pixels[dstIndex + 2] = row[s + 2];
                pixels[dstIndex + 3] = 255;
                dstIndex += 4;
            }
        }
        else
        {
            for (int x = 0; x < width; ++x)
            {
                const int s = x * 4;
                pixels[dstIndex + 0] = row[s + 0];
                pixels[dstIndex + 1] = row[s + 1];
                pixels[dstIndex + 2] = row[s + 2];
                pixels[dstIndex + 3] = row[s + 3];
                dstIndex += 4;
            }
        }
    }

    hasImage = true;
    return true;
}

bool ImageManager::SaveBMP(const char* path)
{
    if (!hasImage || width <= 0 || height <= 0) return false;

    int row = width * 3;
    int pad = (4 - (row % 4)) & 3;
    int stride = row + pad;
    int size = stride * height;

    BITMAPFILEHEADER bmf;
    BITMAPINFOHEADER bih;

    bmf.bfType = 0x4D42;
    bmf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmf.bfSize = bmf.bfOffBits + size;
    bmf.bfReserved1 = 0;
    bmf.bfReserved2 = 0;

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = size;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    std::vector<unsigned char> out;
    out.resize(size);

    for (int y = 0; y < height; ++y)
    {
        int sy = height - 1 - y;
        const unsigned char* srcRow = pixels.data() + sy * width * 4;
        unsigned char* dstRow = out.data() + y * stride;

        for (int x = 0; x < width; ++x)
        {
            int si = x * 4;
            int di = x * 3;
            dstRow[di + 0] = srcRow[si + 0];
            dstRow[di + 1] = srcRow[si + 1];
            dstRow[di + 2] = srcRow[si + 2];
        }

        if (pad)
        {
            unsigned char* padPtr = dstRow + row;
            for (int i = 0; i < pad; ++i) padPtr[i] = 0;
        }
    }

    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE) return false;

    DWORD bw = 0;
    BOOL ok =
        WriteFile(f, &bmf, sizeof(bmf), &bw, 0) &&
        WriteFile(f, &bih, sizeof(bih), &bw, 0) &&
        WriteFile(f, out.data(), (DWORD)out.size(), &bw, 0);

    CloseHandle(f);
    return ok ? true : false;
}

