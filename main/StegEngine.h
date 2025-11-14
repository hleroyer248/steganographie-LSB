#pragma once
#include <string>
#include "ImageManager.h"

class StegEngine
{
public:
    static bool EmbedLSB(ImageManager& img, const std::string& txt);
    static bool ExtractLSB(const ImageManager& img, std::string& out);
};

