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

    if (bih.biCompression != BI_RGB) { CloseHandle(f); return false; }
    if (bih.biBitCount != 24 && bih.biBitCount != 32) { CloseHandle(f); return false; }

    width = bih.biWidth;
    height = bih.biHeight > 0 ? bih.biHeight : -bih.biHeight;

    SetFilePointer(f, bmf.bfOffBits, 0, FILE_BEGIN);

    int bpp = bih.biBitCount / 8;
    int rowRaw = width * bpp;
    int pad = (bpp == 3 ? (4 - (rowRaw % 4)) % 4 : 0);
    int stride = rowRaw + pad;

    std::vector<unsigned char> src(stride * height);
    if (!ReadFile(f, src.data(), (DWORD)src.size(), &br, 0) || br != src.size())
    {
        CloseHandle(f);
        return false;
    }

    CloseHandle(f);

    pixels.assign(width * height * 4, 255);
    bool bottom = bih.biHeight > 0;

    for (int y = 0; y < height; y++)
    {
        int sy = bottom ? (height - 1 - y) : y;
        unsigned char* row = src.data() + sy * stride;
        for (int x = 0; x < width; x++)
        {
            int di = (y * width + x) * 4;
            if (bpp == 3)
            {
                pixels[di] = row[x * 3];
                pixels[di + 1] = row[x * 3 + 1];
                pixels[di + 2] = row[x * 3 + 2];
            }
            else
            {
                pixels[di] = row[x * 4];
                pixels[di + 1] = row[x * 4 + 1];
                pixels[di + 2] = row[x * 4 + 2];
            }
        }
    }

    hasImage = true;
    return true;
}

bool ImageManager::SaveBMP(const char* path)
{
    if (!hasImage) return false;

    int row = width * 3;
    int pad = (4 - (row % 4)) % 4;
    int stride = row + pad;
    int size = stride * height;

    BITMAPFILEHEADER bmf;
    BITMAPINFOHEADER bih;
    ZeroMemory(&bmf, sizeof(bmf));
    ZeroMemory(&bih, sizeof(bih));

    bmf.bfType = 0x4D42;
    bmf.bfOffBits = sizeof(bmf) + sizeof(bih);
    bmf.bfSize = bmf.bfOffBits + size;

    bih.biSize = sizeof(bih);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = size;

    std::vector<unsigned char> out(size, 0);

    for (int y = 0; y < height; y++)
    {
        int sy = height - 1 - y;
        unsigned char* row = out.data() + y * stride;

        for (int x = 0; x < width; x++)
        {
            int si = (sy * width + x) * 4;
            row[x * 3] = pixels[si];
            row[x * 3 + 1] = pixels[si + 1];
            row[x * 3 + 2] = pixels[si + 2];
        }
    }

    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE) return false;

    DWORD bw = 0;
    bool ok =
        WriteFile(f, &bmf, sizeof(bmf), &bw, 0) &&
        WriteFile(f, &bih, sizeof(bih), &bw, 0) &&
        WriteFile(f, out.data(), (DWORD)out.size(), &bw, 0);

    CloseHandle(f);
    return ok;
}
