#include "waku_v2.h"
#include "gd.h"
#include "../image/image.h"

namespace waku_v2 {
    static const int DRAW_AREA_X = 74;
    static const int DRAW_AREA_Y = 70;
    static const int DRAW_AREA_W = 350;
    static const int DRAW_AREA_H = 333;

    namespace {
        bool proc_waku(gd &icon, gd &waku) {
            const int width = waku.width();
            const int height = waku.height();

            icon.alpha(true, true);
            icon.resize_fit(DRAW_AREA_W, DRAW_AREA_H);

            waku.convert_to_true_color();
            waku.alpha(true, true);

            gd built(width, height);
            built.alpha(false, true);
            built.fill_rect(0, 0, width - 1, height - 1, 0x7fffffff);
            built.alpha_blending(true);
            built.copy(icon, DRAW_AREA_X, DRAW_AREA_Y, 0, 0, DRAW_AREA_W, DRAW_AREA_H);
            built.copy(waku, 0, 0, 0, 0, width, height);

            icon.swap(built);
            return true;
        }
    }

    bool waku_v2(gd &icon) {
        gd waku(_binary_waku_v2_png_start, _binary_waku_v2_png_end);
        return proc_waku(icon, waku);
    }
}
