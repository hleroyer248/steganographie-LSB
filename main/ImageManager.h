#pragma once
#include <vector>

class ImageManager
{
public:
    int width;
    int height;
    bool hasImage;
    std::vector<unsigned char> pixels;

    ImageManager();
    bool LoadBMP(const char* path);
    bool SaveBMP(const char* path);
};


