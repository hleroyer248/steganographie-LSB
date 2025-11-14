#pragma once
#include <windows.h>
#include "ImageManager.h"

class RendererGDI
{
public:
    static void Draw(HDC hdc, RECT rc, const ImageManager& img);
};

