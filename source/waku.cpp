#include "waku.h"
#include "gd.h"
#include "../image/image.h"

namespace waku {
    static const int DRAW_AREA_X =  28;
    static const int DRAW_AREA_Y =  27;
    static const int DRAW_AREA_W = 174;
    static const int DRAW_AREA_H = 165;
    static const int DRAW_AREA_BGCOLOR = 0x00bcd77e;

    namespace {
        bool proc(gd &icon, gd &waku) {
            icon.resize_fit(DRAW_AREA_W, DRAW_AREA_H);
            icon.alpha_blending(true);
            icon.save_alpha(false);

            waku.convert_to_true_color();
            waku.alpha_blending(true);
            waku.save_alpha(false);

            gd built(waku.width(), waku.height());
            built.fill(0, 0, DRAW_AREA_BGCOLOR);
            built.alpha_blending(true);
            built.save_alpha(false);
            built.copy(icon, DRAW_AREA_X, DRAW_AREA_Y, 0, 0, icon.width(), icon.height());
            built.copy(waku, 0, 0, 0, 0, waku.width(), waku.height());

            icon.swap(built);
            return true;
        }
    }

    bool waku1(gd &icon) {
        gd waku(_binary_waku1_png_start, _binary_waku1_png_end);
        waku.convert_to_true_color();
        return proc(icon, waku);
    }
}
