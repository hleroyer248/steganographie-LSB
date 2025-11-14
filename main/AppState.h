#pragma once
#include <windows.h>
#include "ImageManager.h"

struct AppState
{
    ImageManager img;
    HWND mainWnd = 0;
    float zoom = 1.0f;
    int offsetX = 0;
    int offsetY = 0;
};

extern AppState g_app;
