#include "waku_v2.h"
#include <string>
#include "gd.h"
#include "manip.h"
#include "../image/image.h"

#include <iostream>

//FIXME
extern std::string config_waku_font_file;

namespace waku_v2 {
    namespace {
        static const int DRAW_AREA_X = 74;
        static const int DRAW_AREA_Y = 70;
        static const int DRAW_AREA_W = 350;
        static const int DRAW_AREA_H = 333;
        static const int SCREEN_NAME_Y_BASELINE      = 433;
        static const int SCREEN_NAME_HEIGHT_LIMIT    = 58;
        static const int SCREEN_NAME_X_LEFT          = 185;
        static const int SCREEN_NAME_WIDTH_LIMIT     = 270;
        static const int SCREEN_NAME_SHADOW_OFFSET_X = 5;
        static const int SCREEN_NAME_SHADOW_OFFSET_Y = 5;

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

        bool draw_screen_name(gd &img, const std::string &screen_name) {
            const int width = img.width();
            const int height = img.height();

            int font_size, text_left, text_top, text_width, text_height;
            if(config_waku_font_file.empty()) {
                std::cerr << "フォントファイルが設定されていません" << std::endl;
                return false;
            }
            for(font_size = SCREEN_NAME_HEIGHT_LIMIT; font_size > 2; --font_size) {
                int rect[8];
                if(!img.ttf_bbox(config_waku_font_file, font_size, 0.0, screen_name, rect)) {
                    return false;
                }
                text_left   = rect[0];
                text_top    = rect[5];
                text_width  = rect[2] - rect[0];
                text_height = rect[1] - rect[5];
                if(-text_top <= SCREEN_NAME_HEIGHT_LIMIT && text_width <= SCREEN_NAME_WIDTH_LIMIT) {
                    break;
                }
            }

            const int x = SCREEN_NAME_X_LEFT + (SCREEN_NAME_WIDTH_LIMIT / 2 - text_width / 2) - text_left;
            const int y = SCREEN_NAME_Y_BASELINE;

            // 文字の影を作成する
            {
                gd shadow(width, height);
                shadow.alpha_blending(true);
                shadow.fill_rect(0, 0, width, height, 0x000000);
                shadow.ttf_draw(
                    config_waku_font_file,
                    font_size, 0.0,
                    screen_name, x + SCREEN_NAME_SHADOW_OFFSET_X, y + SCREEN_NAME_SHADOW_OFFSET_Y,
                    0xffffff
                );
                for(int i = 0; i < 20; ++i) {
                    manip::gaussian_blur(shadow);
                }

                shadow.alpha_blending(true);
                for(int y = 0; y < height; ++y) {
                    for(int x = 0; x < width; ++x) {
                        const int c = (shadow.pixel(x, y) & 0xff) * 4 / 10;
                        if(c > 0) {
                            const int alpha = (0xff - c) * 0x7f / 0xff;
                            img.pixel(x, y, (alpha << 24) | 0x000000);
                        }
                    }
                }
            }

            if(!img.ttf_draw(config_waku_font_file, font_size, 0.0, screen_name, x, SCREEN_NAME_Y_BASELINE, 0xffffff)) {
                return false;
            }

            return true;
        }
    }

    bool waku_v2(gd &icon, const std::string &screen_name) {
        gd waku(_binary_waku_v2_png_start, _binary_waku_v2_png_end);
        return proc_waku(icon, waku) && draw_screen_name(icon, screen_name);
    }
}
