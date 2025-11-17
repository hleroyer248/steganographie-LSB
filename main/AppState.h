#pragma once
#include <windows.h>
#include "ImageManager.h"

struct AppState
{
    ImageManager img;
    HWND mainWnd = 0;
    float zoom = 1.0f; // gere le zoom
    int offsetX = 0; // deplacement de l'image dans la fenetre
    int offsetY = 0;
};

extern AppState g_app; // aces global et partager 
