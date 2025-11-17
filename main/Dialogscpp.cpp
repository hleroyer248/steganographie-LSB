#include "Dialogs.h"
#include "AppState.h"
#include "StegEngine.h"
#include "ImageManager.h"
#include <windows.h>

extern AppState g_app; // recupere les donnes de AppState

static HWND hEditEmbed = 0; //handle global fentre d'insersion et recuperer le texte

const char* cls = "EmbedDlg"; //nom de la classe de fenêtre.
static bool reg = false; //bool statique : permet d’enregistrer la classe une seule fois

void ShowEmbed(HWND parent)
{
    //enregistrement de la classe de fenetre
    if (!reg)
    {
        WNDCLASSA wc; //description complet de la classe
        ZeroMemory(&wc, sizeof(wc)); // remise a zero
        wc.lpfnWndProc = EmbedWndProc;// la fonction gère les évènement de la fenetre
        wc.hInstance = GetModuleHandleA(0); //récupère le handle
        wc.lpszClassName = cls; //creation de la fenetre
        wc.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(32512)); //curseur standard
        RegisterClassA(&wc); //enregistre la classe
        reg = true; //empeche double enregistrement
    }

    HWND dlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME, // fenetre a double dordure
        cls,
        "Inserer un message", 
        WS_CAPTION | WS_POPUPWINDOW, // nom de fentre et fenetre pop-up(contextuelle)
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 200, //taille
        parent, 0, GetModuleHandleA(0), 0
    );

    if (dlg) // affichage du dialogue
    {
        EnableWindow(parent, FALSE); //désactive la fenetre principale
        ShowWindow(dlg, SW_SHOW); //affiche la fenetre avec le dialogue
    }
}

void ShowExtract(HWND parent, const std::string& txt)
{
    

    if (!reg)
    {
        WNDCLASSA wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = ExtractWndProc; //sort le message
        wc.hInstance = GetModuleHandleA(0);
        wc.lpszClassName = cls;
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
        WS_EX_CLIENTEDGE, //bord en retrait
        "EDIT",
        "",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | ES_READONLY, // bordure 3D
        10, 10, 360, 120, //dimention
        dlg, (HMENU)1, GetModuleHandleA(0), 0
    );

    SetWindowTextA(edit, txt.c_str()); //texte extrait placer

    CreateWindowExA(
        0, "BUTTON", "Fermer",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 
        160, 140, 80, 24,
        dlg, (HMENU)2, GetModuleHandleA(0), 0
    );

    ShowWindow(dlg, SW_SHOW); //affichage de la fenetre.
}

LRESULT CALLBACK EmbedWndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_CREATE: // fenetre ou l'utilisateur rentre le message
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
        case 2: // si l'utilisateur clique sur valider
        {
            int len = GetWindowTextLengthA(hEditEmbed);
            std::string txt;
            txt.resize(len);
            if (len > 0) GetWindowTextA(hEditEmbed, &txt[0], len + 1); // +1 car après écriture windows met 0.

            StegEngine::EmbedLSB(g_app.img, txt); // insère le msg dans l'image
            InvalidateRect(g_app.mainWnd, 0, TRUE); //redessine l'image modifier
            EnableWindow(g_app.mainWnd, TRUE); // fermeture de la fenetre de dialogue et activation de la fenetre principale
            DestroyWindow(hWnd);
            return 0;
        }
        case 3: // si il clique sur annuler
            EnableWindow(g_app.mainWnd, TRUE); // activation
            DestroyWindow(hWnd);// destruction 
            return 0;
        }
        break;

    case WM_CLOSE: // si l'utilisateur clique sur la croix 
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
