#include "romanov.h"
#include "gd.h"
#include "../image/image.h"

namespace romanov {
    namespace {
        static const int DRAW_AREA_X = 226;
        static const int DRAW_AREA_Y = 203;
        static const int DRAW_AREA_W =  30;
        static const int DRAW_AREA_H =  30;

        bool proc(gd &icon, gd &romanov) {
            const int width = romanov.width();
            const int height = romanov.height();

            icon.alpha(true, true);
            icon.resize_fit(DRAW_AREA_W, DRAW_AREA_H);

            romanov.convert_to_true_color();
            romanov.alpha(true, true);

            gd built(width, height);
            built.alpha(false, true);
            built.fill_rect(0, 0, width - 1, height - 1, 0x7fffffff);
            built.alpha_blending(true);
            built.copy(icon, DRAW_AREA_X, DRAW_AREA_Y, 0, 0, DRAW_AREA_W, DRAW_AREA_H);
            built.copy(romanov, 0, 0, 0, 0, width, height);

            icon.swap(built);
            return true;
        }
    }

    bool romanov(gd &icon) {
        gd romanov(_binary_romanov_png_start, _binary_romanov_png_end);
        return proc(icon, romanov);
    }
}
