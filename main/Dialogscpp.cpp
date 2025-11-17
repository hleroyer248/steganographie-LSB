#include "Dialogs.h"
#include "AppState.h"
#include "StegEngine.h"
#include "ImageManager.h"
#include <windows.h>

extern AppState g_app; // recupere les donnes de AppState

static HWND hEditEmbed = 0; 

void ShowEmbed(HWND parent)
{
    const char* cls = "EmbedDlg";
    static bool reg = false;

    if (!reg)
    {
        WNDCLASSA wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = EmbedWndProc;
        wc.hInstance = GetModuleHandleA(0);
        wc.lpszClassName = cls;
        wc.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(32512));
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassA(&wc);
        reg = true;
    }

    HWND dlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME,
        cls,
        "Inserer un message",
        WS_CAPTION | WS_POPUPWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 200,
        parent, 0, GetModuleHandleA(0), 0
    );

    if (dlg)
    {
        EnableWindow(parent, FALSE);
        ShowWindow(dlg, SW_SHOW);
    }
}

void ShowExtract(HWND parent, const std::string& txt)
{
    const char* cls = "ExtractDlg";
    static bool reg = false;

    if (!reg)
    {
        WNDCLASSA wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = ExtractWndProc;
        wc.hInstance = GetModuleHandleA(0);
        wc.lpszClassName = cls;
        wc.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(32512));
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassA(&wc);
        reg = true;
    }

    HWND dlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME,
        cls,
        "Message extrait",
        WS_CAPTION | WS_POPUPWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 200,
        parent, 0, GetModuleHandleA(0), 0
    );

    if (!dlg) return;

    EnableWindow(parent, FALSE);

    HWND edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | ES_READONLY,
        10, 10, 360, 120,
        dlg, (HMENU)1, GetModuleHandleA(0), 0
    );

    SetWindowTextA(edit, txt.c_str());

    CreateWindowExA(
        0, "BUTTON", "Fermer",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        160, 140, 80, 24,
        dlg, (HMENU)2, GetModuleHandleA(0), 0
    );

    ShowWindow(dlg, SW_SHOW);
}

LRESULT CALLBACK EmbedWndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_CREATE:
        hEditEmbed = CreateWindowExA(
            WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL,
            10, 10, 360, 120,
            hWnd, (HMENU)1, GetModuleHandleA(0), 0
        );

        CreateWindowExA(0, "BUTTON", "Valider",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            110, 140, 80, 24,
            hWnd, (HMENU)2, GetModuleHandleA(0), 0
        );

        CreateWindowExA(0, "BUTTON", "Annuler",
            WS_CHILD | WS_VISIBLE,
            210, 140, 80, 24,
            hWnd, (HMENU)3, GetModuleHandleA(0), 0
        );
        return 0;

    case WM_COMMAND:
        switch (LOWORD(w))
        {
        case 2:
        {
            int len = GetWindowTextLengthA(hEditEmbed);
            std::string txt;
            txt.resize(len);
            if (len > 0) GetWindowTextA(hEditEmbed, &txt[0], len + 1);

            StegEngine::EmbedLSB(g_app.img, txt);
            InvalidateRect(g_app.mainWnd, 0, TRUE);
            EnableWindow(g_app.mainWnd, TRUE);
            DestroyWindow(hWnd);
            return 0;
        }
        case 3:
            EnableWindow(g_app.mainWnd, TRUE);
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        EnableWindow(g_app.mainWnd, TRUE);
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProcA(hWnd, msg, w, l);
}

LRESULT CALLBACK ExtractWndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(w) == 2)
        {
            EnableWindow(g_app.mainWnd, TRUE);
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        EnableWindow(g_app.mainWnd, TRUE);
        DestroyWindow(hWnd);
        return 0;
    }
    return DefWindowProcA(hWnd, msg, w, l);
}
