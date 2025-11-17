#include "RendererGDI.h"
#include "AppState.h"

extern AppState g_app; // recupere les donnes de AppState

void RendererGDI::Draw(HDC hdc, RECT rc, const ImageManager& img) // dessine l'image chargé
{
   if (!img.hasImage) return; // vérifie qu'une image est bien charger , si ce n'est pas le cas on quitte la fonction.

    int drawW = (int)(img.width * g_app.zoom); // calcul taille finale de l'image en fonction du zoom.(float)
    int drawH = (int)(img.height * g_app.zoom); 

    int x = g_app.offsetX; //placement de l'image
    int y = g_app.offsetY;

    static BITMAPINFO bmi; // créé une seule fois et reutilise à chaque frame.
    static bool init = false; // initialisation dès la premere boule (debut)
    
    //hearder
    if (!init)
    {
        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biXPelsPerMeter = 0;
        bmi.bmiHeader.biYPelsPerMeter = 0;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;
        init = true;
    }

    //parametre qui change
    bmi.bmiHeader.biWidth = img.width; // copie la largeur actuelle
    bmi.bmiHeader.biHeight = -img.height; //(-) permet d'afficher l'image a l'endroit
    bmi.bmiHeader.biSizeImage = 0; // calcul automatiquer par GDI

    SetStretchBltMode(hdc, HALFTONE); //mode de mise a l'echelle(zoom)
    SetBrushOrgEx(hdc, 0, 0, 0); //configuration GDI conseillee par microsoft

    StretchDIBits( // dessin de l'image
        hdc, //zone de dessin
        x,  // position
        y,
        drawW, // taille finale
        drawH,
        0, //point de departs
        0,
        img.width, //taille image source
        img.height,
        img.pixels.data(), //buffer 
        &bmi, // explique a windows
        DIB_RGB_COLORS, // RGB ou RGBA
        SRCCOPY //copie brute
    );
}
