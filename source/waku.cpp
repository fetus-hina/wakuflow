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
            const int width = waku.width();
            const int height = waku.height();

            icon.alpha(true, true);
            icon.resize_fit(DRAW_AREA_W, DRAW_AREA_H);

            waku.convert_to_true_color();
            waku.alpha(true, true);

            gd built(width, height);
            built.alpha(false, true);
            built.fill_rect(0, 0, width - 1, height - 1, 0x7fffffff);
            built.fill_rect(DRAW_AREA_X, DRAW_AREA_Y, DRAW_AREA_X + DRAW_AREA_W, DRAW_AREA_Y + DRAW_AREA_H, DRAW_AREA_BGCOLOR);
            built.alpha_blending(true);
            built.copy(icon, DRAW_AREA_X, DRAW_AREA_Y, 0, 0, DRAW_AREA_W, DRAW_AREA_H);
            built.copy(waku, 0, 0, 0, 0, width, height);

            icon.swap(built);
            return true;
        }
    }

    bool waku1(gd &icon) {
        gd waku(_binary_waku1_png_start, _binary_waku1_png_end);
        return proc(icon, waku);
    }

    bool waku2(gd &icon) {
        gd waku(_binary_waku2_png_start, _binary_waku2_png_end);
        return proc(icon, waku);
    }

    bool waku3(gd &icon) {
        gd waku(_binary_waku3_png_start, _binary_waku3_png_end);
        return proc(icon, waku);
    }

    bool waku4(gd &icon) {
        gd waku(_binary_waku4_png_start, _binary_waku4_png_end);
        return proc(icon, waku);
    }
}
