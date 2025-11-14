#include <windows.h>
#include "AppState.h"
#include "RendererGDI.h"
#include "Dialogs.h"
#include "StegEngine.h"

AppState g_app;

static void DoOpen(HWND hWnd);
static void DoSave(HWND hWnd);
static void DoEmbed(HWND hWnd);
static void DoExtract(HWND hWnd);

static HMENU CreateMenuBar()
{
    HMENU m = CreateMenu();
    HMENU f = CreateMenu();
    HMENU s = CreateMenu();

    AppendMenuA(f, MF_STRING, 1001, "Ouvrir");
    AppendMenuA(f, MF_STRING, 1002, "Enregistrer sous");
    AppendMenuA(f, MF_STRING, 1003, "Quitter");

    AppendMenuA(s, MF_STRING, 1004, "Inserer message");
    AppendMenuA(s, MF_STRING, 1005, "Extraire message");

    AppendMenuA(m, MF_POPUP, (UINT_PTR)f, "Fichier");
    AppendMenuA(m, MF_POPUP, (UINT_PTR)s, "Steganographie");
    return m;
}

static LRESULT CALLBACK MainProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(w))
        {
        case 1001: DoOpen(hWnd); return 0;
        case 1002: DoSave(hWnd); return 0;
        case 1003: PostQuitMessage(0); return 0;
        case 1004: DoEmbed(hWnd); return 0;
        case 1005: DoExtract(hWnd); return 0;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        RendererGDI::Draw(hdc, rc, g_app.img);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_SIZE:
        InvalidateRect(hWnd, 0, TRUE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hWnd, msg, w, l);
}

static void DoOpen(HWND hWnd)
{
    char file[MAX_PATH] = { 0 };

    OPENFILENAMEA of;
    ZeroMemory(&of, sizeof(of));
    of.lStructSize = sizeof(of);
    of.hwndOwner = hWnd;
    of.lpstrFilter = "BMP\0*.bmp\0\0";
    of.lpstrFile = file;
    of.nMaxFile = MAX_PATH;

    if (GetOpenFileNameA(&of))
    {
        if (g_app.img.LoadBMP(file))
            InvalidateRect(hWnd, 0, TRUE);
    }
}

static void AddExtBmp(char* f)
{
    int i = 0;
    while (f[i] != 0) i++;
    f[i] = '.'; f[i + 1] = 'b'; f[i + 2] = 'm'; f[i + 3] = 'p'; f[i + 4] = 0;
}

static void DoSave(HWND hWnd)
{
    if (!g_app.img.hasImage)
    {
        MessageBoxA(hWnd, "Aucune image", "Erreur", MB_OK);
        return;
    }

    char file[MAX_PATH] = { 0 };

    OPENFILENAMEA of;
    ZeroMemory(&of, sizeof(of));
    of.lStructSize = sizeof(of);
    of.hwndOwner = hWnd;
    of.lpstrFilter = "BMP\0*.bmp\0\0";
    of.lpstrFile = file;
    of.nMaxFile = MAX_PATH;
    of.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&of))
    {
        bool hasExt = false;
        for (int i = 0; file[i]; i++)
        {
            if (file[i] == '.') hasExt = true;
        }
        if (!hasExt) AddExtBmp(file);

        g_app.img.SaveBMP(file);
    }
}

static void DoEmbed(HWND hWnd)
{
    if (!g_app.img.hasImage)
    {
        MessageBoxA(hWnd, "Aucune image", "Erreur", MB_OK);
        return;
    }
    ShowEmbed(hWnd);
}

static void DoExtract(HWND hWnd)
{
    if (!g_app.img.hasImage)
    {
        MessageBoxA(hWnd, "Aucune image", "Erreur", MB_OK);
        return;
    }

    std::string out;
    if (!StegEngine::ExtractLSB(g_app.img, out))
    {
        MessageBoxA(hWnd, "Aucun message trouve", "Info", MB_OK);
        return;
    }

    ShowExtract(hWnd, out);
}

int APIENTRY WinMain(HINSTANCE h, HINSTANCE, LPSTR, int cmd)
{
    const char* cls = "MainWndSteg";

    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainProc;
    wc.hInstance = h;
    wc.lpszClassName = cls;
    wc.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(32512));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassExA(&wc);

    HWND w = CreateWindowExA(
        0, cls, "Steganographie BMP",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        0, 0, h, 0
    );

    g_app.mainWnd = w;

    SetMenu(w, CreateMenuBar());
    ShowWindow(w, cmd);
    UpdateWindow(w);

    MSG msg;
    while (GetMessageA(&msg, 0, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}
