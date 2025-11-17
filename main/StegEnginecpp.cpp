#include "StegEngine.h"

bool StegEngine::EmbedLSB(ImageManager& img, const std::string& txt)
{
    if (!img.hasImage) return false;

    const unsigned char m[4] = { 'S','T','E','G' };
    unsigned int len = (unsigned int)txt.size();

    std::vector<unsigned char> payload;
    payload.insert(payload.end(), m, m + 4);
    payload.push_back((len >> 24) & 255);
    payload.push_back((len >> 16) & 255);
    payload.push_back((len >> 8) & 255);
    payload.push_back(len & 255);
    for (char c : txt) payload.push_back((unsigned char)c);

    std::vector<int> bits;
    bits.reserve(payload.size() * 8);
    for (unsigned char c : payload)
        for (int b = 7; b >= 0; b--)
            bits.push_back((c >> b) & 1);

    int pix = img.width * img.height;
    if ((int)bits.size() > pix * 3) return false;

    int bi = 0;
    for (int i = 0; i < pix && bi < (int)bits.size(); i++)
    {
        int base = i * 4;
        for (int c = 0; c < 3 && bi < (int)bits.size(); c++)
        {
            img.pixels[base + c] = (img.pixels[base + c] & 254) | bits[bi];
            bi++;
        }
    }
    return true;
}

bool StegEngine::ExtractLSB(const ImageManager& img, std::string& out)
{
    out.clear();
    if (!img.hasImage) return false;

    int pix = img.width * img.height;
    int maxBytes = (pix * 3) / 8;
    if (maxBytes < 8) return false;

    std::vector<unsigned char> b;
    b.reserve(maxBytes);

    unsigned char cur = 0;
    int pos = 7;

    for (int i = 0; i < pix && (int)b.size() < maxBytes; i++)
    {
        int base = i * 4;
        for (int c = 0; c < 3 && (int)b.size() < maxBytes; c++)
        {
            int bit = img.pixels[base + c] & 1;
            cur |= (bit << pos);
            pos--;
            if (pos < 0)
            {
                b.push_back(cur);
                cur = 0;
                pos = 7;
            }
        }
    }

    if (b.size() < 8) return false;
    if (!(b[0] == 'S' && b[1] == 'T' && b[2] == 'E' && b[3] == 'G')) return false;

    unsigned int len = 0;
    len |= (unsigned int)b[4] << 24;
    len |= (unsigned int)b[5] << 16;
    len |= (unsigned int)b[6] << 8;
    len |= (unsigned int)b[7];

    if (b.size() < 8 + len) return false;

    out.assign((char*)&b[8], (char*)&b[8 + len]);
    return true;
}
 