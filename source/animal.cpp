#include "waku_v2.h"
#include <string>
#include "gd.h"
#include "manip.h"
#include "../image/image.h"

#include <iostream>

//FIXME
extern std::string config_animal_font_file1;
extern std::string config_animal_font_file2;

namespace animal {
    namespace {
        static const int IMAGE_WIDTH         = 600;
        static const int IMAGE_HEIGHT        = 350;
        static const int INNER_WIDTH_LIMIT   = 600 - 40 * 2;
        static const int MAIN_TEXT_BASELINE  = 116;
        static const int MAIN_TEXT_FONT_SIZE =  30;
        static const int SUB_TEXT_BASELINE   = 136;
        static const int SUB_TEXT_FONT_SIZE  =  12;
        static const int INNER_IMAGE_HEIGHT  = 110;
        static const int INNER_IMAGE_TOP     = 156;
        static const int COLOR_FG = 0xe97053;
        static const int COLOR_BG = 0xffffff;

        bool draw_text(gd &dst, const std::string &font, int font_size, const int baseline, const std::string &text) {
            if(font.empty()) {
                std::cerr << "フォントファイルが設定されていません" << std::endl;
                return false;
            }
            int text_width, text_height, text_left, text_top;
            while(font_size > 2) {
                int rect[8];
                if(!dst.ttf_bbox(font, font_size, 0.0, text, rect)) {
                    return false;
                }
                text_width  = rect[2] - rect[0];
                text_height = rect[1] - rect[5];
                text_left   = IMAGE_WIDTH / 2 - text_width / 2;
                text_top    = rect[5];
                if(text_width <= INNER_WIDTH_LIMIT) {
                    break;
                }
                --font_size;
            }
            return dst.ttf_draw(font, font_size, 0.0, text, text_left, baseline, COLOR_FG);
        }

        bool draw_icon(gd &dst, gd &icon) {
            manip::fill_background(icon, 0xffffff);
            icon.resize_fit(INNER_IMAGE_HEIGHT, INNER_IMAGE_HEIGHT); //TODO: 横長画像対応
            if(!manip::grayscale(icon)) {
                return false;
            }
            if(!manip::binarize(icon, true, manip::THRESHOLD_OTSU, manip::DITHERING_NONE)) {
                return false;
            }
            dst.alpha_blending(false);
            const int icon_width = icon.width();
            const int icon_height = icon.height();
            const int x_offset = IMAGE_WIDTH / 2 - icon_width / 2;
            const int y_offset = INNER_IMAGE_TOP + INNER_IMAGE_HEIGHT / 2 - icon_height / 2;
            for(int y = 0; y < icon_height; ++y) {
                for(int x = 0; x < icon_width; ++x) {
                    if((icon.pixel_fast(x, y) & 0xff) < 0x80) {
                        dst.pixel_fast(x_offset + x, y_offset + y, COLOR_FG);
                    }
                }
            }
            return true;
        }
    }

    bool animal(gd &img, const std::string &main_text, const std::string &sub_text) {
        gd dst(IMAGE_WIDTH, IMAGE_HEIGHT);
        dst.fill_rect(0, 0, IMAGE_WIDTH - 1, IMAGE_HEIGHT - 1, COLOR_FG);
        dst.fill_rect(1, 1, IMAGE_WIDTH - 2, IMAGE_HEIGHT - 2, COLOR_BG);
        if(!draw_text(dst, config_animal_font_file1, MAIN_TEXT_FONT_SIZE, MAIN_TEXT_BASELINE, main_text)) {
            return false;
        }
        if(!draw_text(dst, config_animal_font_file2, SUB_TEXT_FONT_SIZE, SUB_TEXT_BASELINE, sub_text)) {
            return false;
        }
        if(!draw_icon(dst, img)) {
            return false;
        }
        img.swap(dst);
        return true;
    }
}
