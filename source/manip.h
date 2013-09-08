#pragma once
#ifndef MANIP_H_
#define MANIP_H_
class gd;
namespace manip {
    bool grayscale(gd &);
    bool colorize(gd &, int r, int g, int b, int a);
    bool binarize(gd &, bool is_grayscaled);
    bool websafe(gd &);
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
