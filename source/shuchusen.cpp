#include "shuchusen.h"
#include "gd.h"
#include "../image/image.h"

namespace shuchusen {
    namespace {
        bool proc(gd &icon, gd &overlay) {
            const int width = overlay.width();
            const int height = overlay.height();
            icon.convert_to_true_color();
            icon.alpha(true, true);
            icon.resize_fit(width, height);
            icon.alpha(true, true);
            overlay.convert_to_true_color();
            overlay.alpha(false, false);
            icon.alpha(true, true);
            icon.copy(overlay, 0, 0, 0, 0, width, height);
            return true;
        }
    }

    bool shuchusen1(gd &icon) {
        gd shuchusen(_binary_shuchusen1_png_start, _binary_shuchusen1_png_end);
        return proc(icon, shuchusen);
    }

    bool shuchusen2(gd &icon) {
        gd shuchusen(_binary_shuchusen2_png_start, _binary_shuchusen2_png_end);
        return proc(icon, shuchusen);
    }

    bool shuchusen3(gd &icon) {
        gd shuchusen(_binary_shuchusen3_png_start, _binary_shuchusen3_png_end);
        return proc(icon, shuchusen);
    }
}
