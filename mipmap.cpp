#include "mipmap.h"

#include "geometry.h"

Mipmap::Mipmap(TGAImage& image) : mipmap_1(TGAImage(image.get_width() / 2, image.get_height() / 2, TGAImage::RGB)) {
    for (int i = 0; i < image.get_width(); i += 2) {
        for (int j = 0; j < image.get_height(); j += 2) {
            Vec4f color1 = toVector4f(image.get(i, j));
            Vec4f color2 = toVector4f(image.get(i, j + 1));
            Vec4f color3 = toVector4f(image.get(i + 1, j));
            Vec4f color4 = toVector4f(image.get(i + 1, j + 1));

            Vec4f res = color1 + color2 + color3 + color4;
            TGAColor result(res[2] / 4, res[1] / 4, res[0] / 4, res[3] / 4);

            mipmap_1.set(i / 2, j / 2, result);
        }
    }
}