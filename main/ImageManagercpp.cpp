#include "ImageManager.h"
#include <windows.h>

ImageManager::ImageManager() : width(0), height(0), hasImage(false) //initialisation des valeurs (constructeur)
{
}
// suite de condition a l'ouverture de l'image

bool ImageManager::LoadBMP(const char* path)
{
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE) return false; // ouvre un fichier BMP avec droit d'acces standard

    DWORD br = 0;
    BITMAPFILEHEADER bmf;
    if (!ReadFile(f, &bmf, sizeof(bmf), &br, 0) || br != sizeof(bmf)) // lit le fichier , si il ne peux pas lire(ou lit partiellement) alors le fichier se ferme (CloseHandle)
    {
        CloseHandle(f);
        return false;
    }
    if (bmf.bfType != 0x4D42) //bfType doit être 0x4D42, ce qui correspond à "BM" en ASCII. 
    {
        CloseHandle(f);
        return false;
    }

    BITMAPINFOHEADER bih;
    if (!ReadFile(f, &bih, sizeof(bih), &br, 0) || br != sizeof(bih)) // lecture complete de la structure du fichier BMP (bih = BIT DEPTH)
    {
        CloseHandle(f);
        return false;
    }

    if (bih.biCompression != BI_RGB)  // aucune compression
    {
        CloseHandle(f);
        return false;
    }
    if (bih.biBitCount != 24 && bih.biBitCount != 32) // verifie que l'image est bien en 24 bits/pixel (RGB) ou 32 bits/pixel (BGRA)
    {
        CloseHandle(f);
        return false;
    }

    //coordonee
    int w = bih.biWidth; 
    int h = bih.biHeight > 0 ? bih.biHeight : -bih.biHeight;

    if (w <= 0 || h <= 0) // verifie que longueur et largeur soit positif
    {
        CloseHandle(f);
        return false;
    }

    //On fixe les champs membres width et height de l’objet.
    width = w;
    height = h;

    //Positionnement du curseur sur les données pixels
    SetFilePointer(f, bmf.bfOffBits, 0, FILE_BEGIN); // offset dans le fichier à partir duquel commencent réellement les pixels (offset = déplacement en mémoire)

    //calcul du stride
    int bpp = bih.biBitCount / 8; // bpp = bytes par pixel 
    int rowRaw = width * bpp; // nombre d’octets réels pour les pixels sur une ligne 
    int pad = (bpp == 3) ? ((4 - (rowRaw % 4)) & 3) : 0;  // alllignement de la ligne sur 4octets ((bpp == 3) = fichier BMP 24bits
    int stride = rowRaw + pad; // taille total de chaque lignes du fichier.

    const int totalSize = stride * height;  //taille de limage
    std::vector<unsigned char> src;           // lecture de tout le bloc de pixels dans un buffer temporaire (src)
    src.resize(totalSize);

    if (!ReadFile(f, src.data(), (DWORD)totalSize, &br, 0) || br != (DWORD)totalSize) // si lecture impossible
    {
        CloseHandle(f);
        return false;
    }

   // CloseHandle(f); //fermeture du fichier

    const int pixCount = width * height; //total de pixel
    pixels.clear(); // on efface
    pixels.resize(pixCount * 4); //stockage de 4 octet par pixel sur BGRA

    const bool bottom = bih.biHeight > 0; // si l'image est a l'envers

    for (int y = 0; y < height; ++y) //conversion BGRA
    {
        //parcours de la ligne y du buffer interne
        int sy = bottom ? (height - 1 - y) : y; // si l'image est a l'envers alors on la retourne
        const unsigned char* row = src.data() + sy * stride; // pointeur vers le début de la ligne dans src
        int dstIndex = y * width * 4; //index dans pixels pour le début de la ligne

        if (bpp == 3) //si image en 24bits
        {
            for (int x = 0; x < width; ++x)
            {
                const int s = x * 3;
                pixels[dstIndex + 0] = row[s + 0]; // lecture RGB puis placer dans pixels
                pixels[dstIndex + 1] = row[s + 1];
                pixels[dstIndex + 2] = row[s + 2];
                pixels[dstIndex + 3] = 255; //on ajoute alpha
                dstIndex += 4; // avance de 4 a chaquez pixel
            }
        }
        else
        {
            for (int x = 0; x < width; ++x)
            {
                const int s = x * 4;
                pixels[dstIndex + 0] = row[s + 0]; //copie tel quel dans buffer interne
                pixels[dstIndex + 1] = row[s + 1];
                pixels[dstIndex + 2] = row[s + 2];
                pixels[dstIndex + 3] = row[s + 3];
                dstIndex += 4;
            }
        }
    }

    hasImage = true; 
    return true;
}

bool ImageManager::SaveBMP(const char* path)
{
    if (!hasImage || width <= 0 || height <= 0) return false; // vérifie qu'une image est chargee

    int row = width * 3; // octets par ligne
    int pad = (4 - (row % 4)) & 3; //padding (ligne multiple de 4 pixel)
    int stride = row + pad; //longueur total d'une ligne avec le padding
    int size = stride * height; // taille totale 

    BITMAPFILEHEADER bmf; //header
    BITMAPINFOHEADER bih;

    bmf.bfType = 0x4D42; //type
    bmf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmf.bfSize = bmf.bfOffBits + size; // taille totale
    bmf.bfReserved1 = 0;
    bmf.bfReserved2 = 0;

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = size;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;
    // Header d’info BMP classique 24 bits, sans compression.

    //buffer de sortie
    std::vector<unsigned char> out;
    out.resize(size); //out contiend les pixels en format BMP 24 bits + padding.

    for (int y = 0; y < height; ++y)// parcourt les lignes
    {
        int sy = height - 1 - y; // lignes source
        const unsigned char* srcRow = pixels.data() + sy * width * 4; // pointe la ligne 
        unsigned char* dstRow = out.data() + y * stride; //pointe la sortie

        for (int x = 0; x < width; ++x)
        {
            //copie de BRG, ignore Alpha
            int si = x * 4;
            int di = x * 3;
            dstRow[di + 0] = srcRow[si + 0];
            dstRow[di + 1] = srcRow[si + 1];
            dstRow[di + 2] = srcRow[si + 2];
        }

        if (pad) // rempli octet de padding avec zero
        {
            unsigned char* padPtr = dstRow + row;
            for (int i = 0; i < pad; ++i) padPtr[i] = 0;
        }
    }
    // ecris dans le fichier , "create always" crée un fichier meme si il en existe deja un (il tronqe le fichier existant)
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (f == INVALID_HANDLE_VALUE) return false;
    
    DWORD bw = 0;
    BOOL ok =
        WriteFile(f, &bmf, sizeof(bmf), &bw, 0) &&
        WriteFile(f, &bih, sizeof(bih), &bw, 0) &&
        WriteFile(f, out.data(), (DWORD)out.size(), &bw, 0);

    CloseHandle(f);
    return ok ? true : false;
}

