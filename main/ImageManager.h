#pragma once
#include <vector>

class ImageManager
{
public:
    int width;
    int height;
    bool hasImage; // verifie qu'une image est charger
    std::vector<unsigned char> pixels; // placement des pixels

    ImageManager();
    bool LoadBMP(const char* path); //fonction chargement de l'image
    bool SaveBMP(const char* path); //fonction sauvegarde de l'image
};


