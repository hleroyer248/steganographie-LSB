#pragma once
#include <string>
#include "ImageManager.h"

class StegEngine
{
public:
    static bool EmbedLSB(ImageManager& img, const std::string& txt); //dissimuler le message dans l'image
    static bool ExtractLSB(const ImageManager& img, std::string& out); //sortir le message caché dans l'image
};

