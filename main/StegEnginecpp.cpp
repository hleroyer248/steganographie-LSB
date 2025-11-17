#include "StegEngine.h"

bool StegEngine::EmbedLSB(ImageManager& img, const std::string& txt) //dissimuler le message dans l'image
{
    if (!img.hasImage) return false; // vérification d'une image chage

    const unsigned char m[4] = { 'S','T','E','G' };//signature reconnaissable lors de l’extraction
    unsigned int len = (unsigned int)txt.size(); //longueur du message à cacher (en octets)

    std::vector<unsigned char> payload;
    payload.insert(payload.end(), m, m + 4); // ajout de 4 octets a MAGIC (m)
    payload.push_back((len >> 24) & 255);// Format big-endian classique
    payload.push_back((len >> 16) & 255);
    payload.push_back((len >> 8) & 255);
    payload.push_back(len & 255);
    for (char c : txt) payload.push_back((unsigned char)c); //ajout des characteres du messages

    //conversion payload en tableau bainaire
    std::vector<int> bits;
    bits.reserve(payload.size() * 8);
    for (unsigned char c : payload)
        for (int b = 7; b >= 0; b--)
            bits.push_back((c >> b) & 1);

    //vérifie la capacityé de l"image
    int pix = img.width * img.height; 
    if ((int)bits.size() > pix * 3) return false;

    int bi = 0;// insersion des pixels
    for (int i = 0; i < pix && bi < (int)bits.size(); i++)
    {                                                             // i : index du pixel (0 -> pix-1), base : index du pixel dans le buffer RGBA
        int base = i * 4;
        for (int c = 0; c < 3 && bi < (int)bits.size(); c++)
        {
            img.pixels[base + c] = (img.pixels[base + c] & 254) | bits[bi]; // efface le dernier bits puis le remplace.
            bi++;
        }
    }
    return true;
}

bool StegEngine::ExtractLSB(const ImageManager& img, std::string& out)
{
    out.clear();
    if (!img.hasImage) return false;

    int pix = img.width * img.height; // capacite maximale en octets
    int maxBytes = (pix * 3) / 8;
    if (maxBytes < 8) return false;

    std::vector<unsigned char> b; //buffer de sortie
    b.reserve(maxBytes);

    unsigned char cur = 0; //octets en cours de reconstruction
    int pos = 7; // commence par le bit de poid fort

    for (int i = 0; i < pix && (int)b.size() < maxBytes; i++)
    {
        int base = i * 4;
        for (int c = 0; c < 3 && (int)b.size() < maxBytes; c++) //extraction de 1 bit dans R,G,B
        {
            int bit = img.pixels[base + c] & 1;
            cur |= (bit << pos); //placement du bit
            pos--;
            if (pos < 0) // quand l'octet est rempli 
            {
                b.push_back(cur); // reconstitution de l'octet dans b, puis remise des paramètre a 0 pour le prochain
                cur = 0;
                pos = 7;
            }
        }
    }

    //verification de header
    if (b.size() < 8) return false; 
    if (!(b[0] == 'S' && b[1] == 'T' && b[2] == 'E' && b[3] == 'G')) return false; // si aucun message cache

    unsigned int len = 0; //lecture de la longueur
    len |= (unsigned int)b[4] << 24;
    len |= (unsigned int)b[5] << 16;
    len |= (unsigned int)b[6] << 8;
    len |= (unsigned int)b[7];

    if (b.size() < 8 + len) return false; // verifie que le message est complet

    out.assign((char*)&b[8], (char*)&b[8 + len]); //lecture des octet dans b
    return true;
}
 