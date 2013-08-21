#include "stamp.h"
#include "gd.h"
#include "../image/image.h"

namespace stamp {
    namespace {
        bool proc(gd &icon, gd &stamp) {
            const int width = stamp.width();
            const int height = stamp.height();
            icon.convert_to_true_color();
            icon.alpha(true, true);
            icon.resize_fit(width, height);
            icon.alpha(true, true);
            stamp.convert_to_true_color();
            stamp.alpha(false, false);
            icon.alpha(true, true);
            icon.copy(stamp, 0, 0, 0, 0, width, height);
            return true;
        }
    }

    bool half_price(gd &icon) {
        gd stamp(_binary_half_price_png_start, _binary_half_price_png_end);
        return proc(icon, stamp);
    }

    bool kankore_half_damage(gd &icon) {
        gd stamp(_binary_kankore_half_damage_png_start, _binary_kankore_half_damage_png_end);
        return proc(icon, stamp);
    }
}
