#pragma once
#include "ImageManager.h"
#include <windows.h>

struct AppState
{
    ImageManager img;
    HWND mainWnd;
};

extern AppState g_app;


