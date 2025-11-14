#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <string>

using namespace std;

static constexpr int ID_MENU_OPEN = 1001;
static constexpr int ID_MENU_SAVEAS = 1002;
static constexpr int ID_MENU_EXIT = 1003;
static constexpr int ID_MENU_EMBED = 1004;
static constexpr int ID_MENU_EXTRACT = 1005;

class ImageManager
{
public:
    int width;
    int height;
    bool hasImage;
    vector<unsigned char> pixels;

    ImageManager() : width(0), height(0), hasImage(false)
    {
    }

    bool LoadBMP(const char* path)
    {
        HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD br = 0;
        BITMAPFILEHEADER bmf;
        if (!ReadFile(hFile, &bmf, sizeof(bmf), &br, 0) || br != sizeof(bmf))
        {
            CloseHandle(hFile);
            return false;
        }
        if (bmf.bfType != 0x4D42)
        {
            CloseHandle(hFile);
            return false;
        }

        BITMAPINFOHEADER bih;
        if (!ReadFile(hFile, &bih, sizeof(bih), &br, 0) || br != sizeof(bih))
        {
            CloseHandle(hFile);
            return false;
        }

        if (bih.biCompression != BI_RGB)
        {
            CloseHandle(hFile);
            return false;
        }
        if (bih.biBitCount != 24 && bih.biBitCount != 32)
        {
            CloseHandle(hFile);
            return false;
        }

        width = bih.biWidth;
        height = bih.biHeight > 0 ? bih.biHeight : -bih.biHeight;

        if (SetFilePointer(hFile, bmf.bfOffBits, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            CloseHandle(hFile);
            return false;
        }

        int bpp = bih.biBitCount / 8;
        int rowRaw = width * bpp;
        int pad = (bpp == 3) ? ((4 - (rowRaw % 4)) % 4) : 0;
        int stride = rowRaw + pad;

        vector<unsigned char> src(stride * height);
        if (!ReadFile(hFile, src.data(), (DWORD)src.size(), &br, 0) || br != src.size())
        {
            CloseHandle(hFile);
            return false;
        }

        CloseHandle(hFile);

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

    bool SaveBMP(const char* path)
    {
        if (!hasImage || width <= 0 || height <= 0) return false;

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

        vector<unsigned char> out(size, 0);

        for (int y = 0; y < height; y++)
        {
            int sy = height - 1 - y;
            unsigned char* rowPtr = out.data() + y * stride;
            for (int x = 0; x < width; x++)
            {
                int si = (sy * width + x) * 4;
                rowPtr[x * 3] = pixels[si];
                rowPtr[x * 3 + 1] = pixels[si + 1];
                rowPtr[x * 3 + 2] = pixels[si + 2];
            }
        }

        HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        DWORD bw = 0;
        bool ok = true;
        if (!WriteFile(hFile, &bmf, sizeof(bmf), &bw, 0) || bw != sizeof(bmf)) ok = false;
        if (ok && (!WriteFile(hFile, &bih, sizeof(bih), &bw, 0) || bw != sizeof(bih))) ok = false;
        if (ok && (!WriteFile(hFile, out.data(), (DWORD)out.size(), &bw, 0) || bw != out.size())) ok = false;

        CloseHandle(hFile);
        return ok;
    }
};

class RendererGDI
{
public:
    static void Draw(HDC hdc, RECT rc, const ImageManager& img)
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
};

class StegEngine
{
public:
    static bool EmbedLSB(ImageManager& img, const string& txt)
    {
        if (!img.hasImage) return false;

        const unsigned char m[4] = { 'S', 'T', 'E', 'G' };
        unsigned int len = (unsigned int)txt.size();

        vector<unsigned char> payload;
        payload.reserve(8 + txt.size());
        payload.insert(payload.end(), m, m + 4);
        payload.push_back((unsigned char)((len >> 24) & 255));
        payload.push_back((unsigned char)((len >> 16) & 255));
        payload.push_back((unsigned char)((len >> 8) & 255));
        payload.push_back((unsigned char)(len & 255));
        for (char c : txt) payload.push_back((unsigned char)c);

        vector<int> bits;
        bits.reserve(payload.size() * 8);
        for (unsigned char c : payload)
        {
            for (int b = 7; b >= 0; b--)
            {
                bits.push_back((c >> b) & 1);
            }
        }

        int pix = img.width * img.height;
        if ((int)bits.size() > pix * 3) return false;

        int bi = 0;
        for (int i = 0; i < pix && bi < (int)bits.size(); i++)
        {
            int base = i * 4;
            for (int c = 0; c < 3 && bi < (int)bits.size(); c++)
            {
                img.pixels[base + c] = (unsigned char)((img.pixels[base + c] & 254) | (bits[bi] & 1));
                bi++;
            }
        }

        return true;
    }

    static bool ExtractLSB(const ImageManager& img, string& out)
    {
        out.clear();
        if (!img.hasImage) return false;

        int pix = img.width * img.height;
        int maxBytes = (pix * 3) / 8;
        if (maxBytes < 8) return false;

        vector<unsigned char> bytes;
        bytes.reserve(maxBytes);

        unsigned char cur = 0;
        int pos = 7;

        for (int i = 0; i < pix && (int)bytes.size() < maxBytes; i++)
        {
            int base = i * 4;
            for (int c = 0; c < 3 && (int)bytes.size() < maxBytes; c++)
            {
                int bit = img.pixels[base + c] & 1;
                cur |= (unsigned char)(bit << pos);
                pos--;
                if (pos < 0)
                {
                    bytes.push_back(cur);
                    cur = 0;
                    pos = 7;
                }
            }
        }

        if (bytes.size() < 8) return false;
        if (!(bytes[0] == 'S' && bytes[1] == 'T' && bytes[2] == 'E' && bytes[3] == 'G')) return false;

        unsigned int len = 0;
        len |= (unsigned int)bytes[4] << 24;
        len |= (unsigned int)bytes[5] << 16;
        len |= (unsigned int)bytes[6] << 8;
        len |= (unsigned int)bytes[7];

        if ((unsigned int)bytes.size() < 8 + len) return false;

        out.reserve(len);
        for (unsigned int i = 0; i < len; i++)
        {
            out.push_back((char)bytes[8 + i]);
        }

        return true;
    }
};

struct AppState
{
    ImageManager img;
    HWND mainWnd;
} g_app;

static LRESULT CALLBACK EmbedWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK ExtractWndProc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK MainProc(HWND, UINT, WPARAM, LPARAM);

static void DoOpen(HWND hWnd)
{
    char file[MAX_PATH] = { 0 };
    OPENFILENAMEA of;
    ZeroMemory(&of, sizeof(of));
    of.lStructSize = sizeof(of);
    of.hwndOwner = hWnd;
    of.lpstrFilter = "BMP\0*.bmp\0Tous\0*.*\0\0";
    of.lpstrFile = file;
    of.nMaxFile = MAX_PATH;
    of.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileNameA(&of))
    {
        if (!g_app.img.LoadBMP(file))
        {
            MessageBoxA(hWnd, "Impossible de charger l image", "Erreur", MB_OK);
        }
        else
        {
            InvalidateRect(hWnd, 0, TRUE);
        }
    }
}

static void ForceBmpExtension(char* path, int maxSize)
{
    int len = 0;
    while (len < maxSize && path[len] != 0)
    {
        len++;
    }
    if (len == 0) return;

    int lastSlash = -1;
    int lastDot = -1;
    for (int i = 0; i < len; i++)
    {
        if (path[i] == '\\' || path[i] == '/')
            lastSlash = i;
        else if (path[i] == '.')
            lastDot = i;
    }

    bool hasExt = (lastDot > lastSlash);
    if (!hasExt)
    {
        if (len + 4 < maxSize)
        {
            path[len + 0] = '.';
            path[len + 1] = 'b';
            path[len + 2] = 'm';
            path[len + 3] = 'p';
            path[len + 4] = 0;
        }
    }
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
    of.lpstrFilter = "BMP\0*.bmp\0Tous\0*.*\0\0";
    of.lpstrFile = file;
    of.nMaxFile = MAX_PATH;
    of.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    of.lpstrDefExt = "bmp";

    if (GetSaveFileNameA(&of))
    {
        ForceBmpExtension(file, MAX_PATH);
        if (!g_app.img.SaveBMP(file))
        {
            MessageBoxA(hWnd, "Erreur sauvegarde", "Erreur", MB_OK);
        }
    }
}

static void ShowEmbed(HWND parent)
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
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(32512));
        RegisterClassA(&wc);
        reg = true;
    }

    HWND dlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME,
        cls,
        "Inserer un message",
        WS_CAPTION | WS_POPUPWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        200,
        parent,
        0,
        GetModuleHandleA(0),
        0
    );

    if (dlg)
    {
        EnableWindow(parent, FALSE);
        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
    }
}

static void ShowExtract(HWND parent, const string& txt)
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
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(32512));
        RegisterClassA(&wc);
        reg = true;
    }

    HWND dlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME,
        cls,
        "Message extrait",
        WS_CAPTION | WS_POPUPWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        400,
        200,
        parent,
        0,
        GetModuleHandleA(0),
        0
    );

    if (dlg)
    {
        EnableWindow(parent, FALSE);

        HWND edit = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "EDIT",
            "",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
            10,
            10,
            360,
            120,
            dlg,
            (HMENU)1,
            GetModuleHandleA(0),
            0
        );

        SetWindowTextA(edit, txt.c_str());

        CreateWindowExA(
            0,
            "BUTTON",
            "Fermer",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            160,
            140,
            80,
            24,
            dlg,
            (HMENU)2,
            GetModuleHandleA(0),
            0
        );

        ShowWindow(dlg, SW_SHOW);
        UpdateWindow(dlg);
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

    string out;
    if (!StegEngine::ExtractLSB(g_app.img, out))
    {
        MessageBoxA(hWnd, "Aucun message trouve", "Info", MB_OK);
        return;
    }

    ShowExtract(hWnd, out);
}

static HMENU CreateMenuBar()
{
    HMENU hMain = CreateMenu();
    HMENU hFile = CreateMenu();
    HMENU hSteg = CreateMenu();

    AppendMenuA(hFile, MF_STRING, ID_MENU_OPEN, "Ouvrir");
    AppendMenuA(hFile, MF_STRING, ID_MENU_SAVEAS, "Enregistrer sous");
    AppendMenuA(hFile, MF_STRING, ID_MENU_EXIT, "Quitter");

    AppendMenuA(hSteg, MF_STRING, ID_MENU_EMBED, "Inserer message");
    AppendMenuA(hSteg, MF_STRING, ID_MENU_EXTRACT, "Extraire message");

    AppendMenuA(hMain, MF_POPUP, (UINT_PTR)hFile, "Fichier");
    AppendMenuA(hMain, MF_POPUP, (UINT_PTR)hSteg, "Steganographie");

    return hMain;
}

static LRESULT CALLBACK EmbedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hEdit = 0;

    switch (msg)
    {
    case WM_CREATE:
        hEdit = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "EDIT",
            "",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL,
            10,
            10,
            360,
            120,
            hWnd,
            (HMENU)1,
            GetModuleHandleA(0),
            0
        );

        CreateWindowExA(
            0,
            "BUTTON",
            "Valider",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            110,
            140,
            80,
            24,
            hWnd,
            (HMENU)2,
            GetModuleHandleA(0),
            0
        );

        CreateWindowExA(
            0,
            "BUTTON",
            "Annuler",
            WS_CHILD | WS_VISIBLE,
            210,
            140,
            80,
            24,
            hWnd,
            (HMENU)3,
            GetModuleHandleA(0),
            0
        );
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 2:
        {
            int len = GetWindowTextLengthA(hEdit);
            string txt;
            txt.resize(len);
            if (len > 0)
            {
                GetWindowTextA(hEdit, &txt[0], len + 1);
            }

            if (!StegEngine::EmbedLSB(g_app.img, txt))
            {
                MessageBoxA(hWnd, "Echec insertion", "Erreur", MB_OK);
            }
            else
            {
                InvalidateRect(g_app.mainWnd, 0, TRUE);
            }

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

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK ExtractWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == 2)
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

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static LRESULT CALLBACK MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_MENU_OPEN:
            DoOpen(hWnd);
            return 0;
        case ID_MENU_SAVEAS:
            DoSave(hWnd);
            return 0;
        case ID_MENU_EXIT:
            PostQuitMessage(0);
            return 0;
        case ID_MENU_EMBED:
            DoEmbed(hWnd);
            return 0;
        case ID_MENU_EXTRACT:
            DoExtract(hWnd);
            return 0;
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
    }
    return 0;

    case WM_SIZE:
        InvalidateRect(hWnd, 0, TRUE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    const char* cls = "MainWndSteg";

    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainProc;
    wc.hInstance = hInst;
    wc.lpszClassName = cls;
    wc.hCursor = LoadCursorA(0, MAKEINTRESOURCEA(32512));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassExA(&wc))
    {
        return 0;
    }

    HWND hWnd = CreateWindowExA(
        0,
        cls,
        "Steganographie BMP",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        0,
        0,
        hInst,
        0
    );

    if (!hWnd)
    {
        return 0;
    }

    g_app.mainWnd = hWnd;

    HMENU hMenu = CreateMenuBar();
    SetMenu(hWnd, hMenu);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageA(&msg, 0, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

