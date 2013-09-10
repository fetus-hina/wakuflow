#pragma once
#ifndef MANIP_H_
#define MANIP_H_
class gd;
namespace manip {
    enum DITHERING_METHOD {
        DITHERING_NONE,
        DITHERING_FLOYD_STEINBERG,
        DITHERING_SIERRA_3LINE,
        DITHERING_SIERRA_2LINE,
        DITHERING_SIERRA_LITE,
        DITHERING_ATKINSON,
    };

    bool grayscale(gd &);
    bool colorize(gd &, int r, int g, int b, int a);
    bool binarize(gd &, bool is_grayscaled);
    bool websafe(gd &, DITHERING_METHOD m);
    bool famicom(gd &, DITHERING_METHOD m);
    bool negate(gd &);
    bool pixelate(gd &, int);
    bool emboss(gd &);
    bool gaussian_blur(gd &);
    bool sharpen(gd &);
    bool edge(gd &);
    bool rotate_fast(gd &, int degree);
    bool flip_horizontal(gd &);
    bool flip_vertical(gd &);
}
#endif
