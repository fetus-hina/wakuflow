#include "manip.h"
#include "gd.h"
#include "../image/image.h"

namespace manip {
    namespace {
        struct grayscale_method_avg {
            int operator()(int color) const {
                const int alpha = (color & 0x7f000000);
                const int r = (color & 0x00ff0000) >> 16;
                const int g = (color & 0x0000ff00) >>  8;
                const int b = (color & 0x000000ff);
                const int c = static_cast<int>((r + g + b) / 3.0 + 0.5);
                return alpha | (c * 0x010101);
            }
        };

        struct grayscale_method_ntsc {
            int operator()(int color) const {
                const int alpha = (color & 0x7f000000);
                const int r = (color & 0x00ff0000) >> 16;
                const int g = (color & 0x0000ff00) >>  8;
                const int b = (color & 0x000000ff);
                const int c = static_cast<int>(r * 0.299 + g * 0.587 + b * 0.114 + 0.5);
                return alpha | (c * 0x010101);
            }
        };

        template<typename T>
        bool grayscale_impl(gd &img, const T &method) {
            img.alpha_blending(false);
            const int width = img.width();
            const int height = img.height();
            for(int y = 0; y < height; ++y) {
                for(int x = 0; x < width; ++x) {
                    img.pixel_fast(x, y, method(img.pixel_fast(x, y)));
                }
            }
            return true;
        }
    }

    bool grayscale(gd &img, GRAYSCALE_METHOD me) {
        switch(me) {
        case GRAY_AVERAGE:
            return grayscale_impl(img, grayscale_method_avg());
        case GRAY_NTSC:
        default:
            return grayscale_impl(img, grayscale_method_ntsc());
        }
    }
}
