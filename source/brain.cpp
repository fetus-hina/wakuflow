#include "waku_v2.h"
#include <string>
#include "gd.h"
#include "manip.h"
#include "../image/image.h"

#include <iostream>

//FIXME
extern std::string config_animal_font_file1;

namespace brain {
    namespace {
        static const int IMAGE_WIDTH        = 470;
        static const int IMAGE_HEIGHT       = 500;

        static const int TEXT_AREA_WIDTH    = 332;
        static const int TEXT_AREA_HEIGHT   =  90;
        static const int TEXT_AREA_X        = 470 / 2 - 332 / 2;
        static const int TEXT_AREA_Y        =  64;

        static const int MAX_FONT_SIZE      =  30;

        bool draw_text(gd &dst, const std::string &font, const std::string &text) {
            if(font.empty()) {
                std::cerr << "フォントファイルが設定されていません" << std::endl;
                return false;
            }
            int font_size = MAX_FONT_SIZE;
            int text_left;
            int baseline;
            while (font_size > 2) {
                int rect[8];
                if(!dst.ttf_bbox(font, font_size, 0.0, text, rect)) {
                    return false;
                }
                const int text_width  = rect[2] - rect[0];
                const int text_height = rect[1] - rect[7];
                const int text_top = TEXT_AREA_Y + (TEXT_AREA_HEIGHT / 2 - text_height / 2);
                baseline = text_top - rect[7];
                text_left = TEXT_AREA_X + (TEXT_AREA_WIDTH / 2 - text_width / 2);
                if(text_width <= TEXT_AREA_WIDTH && text_height <= TEXT_AREA_HEIGHT) {
                    break;
                }
                --font_size;
            }

            return dst.ttf_draw(font, font_size, 0.0, text, text_left, baseline, 0x000000);
        }
    }

    bool brain(gd &img, const std::string &text) {
        gd dst(_binary_brain_png_start, _binary_brain_png_end);
        if(!draw_text(dst, config_animal_font_file1, text)) {
            return false;
        }
        img.swap(dst);
        return true;
    }
}
