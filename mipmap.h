#ifndef MIPMAP_H
#define MIPMAP_H
#include "tgaimage.h"

class Mipmap {
public:
    Mipmap() = default;
    Mipmap(TGAImage& image);

    TGAImage& getMipmap() { return mipmap_1; }

private:
    TGAImage mipmap_1;
};

#endif