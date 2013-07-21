#include "manip.h"
#include "gd.h"
#include "../image/image.h"

namespace manip {
    bool grayscale(gd &img) {
        img.convert_to_true_color();
        img.alpha_blending(false);
        const int width = img.width();
        const int height = img.height();
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int r = (color & 0xff0000) >> 16;
                const int g = (color & 0x00ff00) >>  8;
                const int b = (color & 0x0000ff);
                const int a = (color & 0x7f000000); // そのまま使うのでビットシフトしない
                const int gray = (77 * r + 150 * g + 29 * b + 128) / 256; // 0.298912 * r + 0.586611 * g + 0.114478 * b + 0.5
                img.pixel_fast(x, y, a | (gray * 0x010101));
            }
        }
        return true;
    }

    bool colorize(gd &img, int red, int green, int blue, int alpha) {
        img.convert_to_true_color();
        img.alpha_blending(false);
        const int width = img.width();
        const int height = img.height();
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int r = ((color & 0xff0000) >> 16) + red;
                const int g = ((color & 0x00ff00) >>  8) + green;
                const int b = (color & 0x0000ff) + blue;
                const int a = ((color & 0x7f000000) >> 24) + alpha;
                const int new_color = 
                    ((a < 0 ? 0 : a > 127 ? 127 : a) << 24) |
                    ((r < 0 ? 0 : r > 255 ? 255 : r) << 16) |
                    ((g < 0 ? 0 : g > 255 ? 255 : g) <<  8) |
                     (b < 0 ? 0 : b > 255 ? 255 : b);
                img.pixel_fast(x, y, new_color);
            }
        }
        return true;
    }
}
