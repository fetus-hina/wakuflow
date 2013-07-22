#include "manip.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include "gd.h"
#include "../image/image.h"

namespace manip {
    namespace {
        // 大津の手法による閾値の計算
        int otsu_threshold(const double average, const int histgram[256]) {
            double max = 0.;
            int max_no = 0;
            for(int i = 0; i < 256; ++i) {
                long count1 = 0, count2 = 0;
                long long data = 0;
                double breakup1 = 0., breakup2 = 0.;
                double average1 = 0., average2 = 0.;
                for(int j = 0; j < i; ++j) {
                    count1 += histgram[j];
                    data += histgram[j] * j;
                }
                if(count1 > 0) {
                    average1 = (double)data / (double)count1;
                    for(int j = 0; j < i; ++j) {
                        breakup1 += pow(j - average1, 2) * histgram[j];
                    }
                    breakup1 /= (double)count1;
                }

                data = 0;
                for(int j = i; j < 256; ++j) {
                    count2 += histgram[j];
                    data += histgram[j] * j;
                }
                if(count2 > 0) {
                    average2 = (double)data / (double)count2;
                    for(int j = i; j < 256; ++j) {
                        breakup2 += pow(j - average2, 2) * histgram[j];
                    }
                    breakup2 /= (double)count2;
                }
                const double class1 = (double)count1 * breakup1 + (double)count2 * breakup2;
                const double class2 = (double)count1 * pow(average1 - average, 2) + (double)count2 * pow(average2 - average, 2);
                const double tmp = class2 / class1;
                if(max < tmp) {
                    max = tmp;
                    max_no = i;
                }
            }
            return max_no;
        }

        // 3x3 微分オペレータを適用する
        bool apply_operator_3x3(gd &img, const double op[3][3], double filter_div, double offset) {
            const int width = img.width();
            const int height = img.height();
            gd dst(width, height);
            dst.alpha_blending(false);

            for(int y = 0; y < height; ++y) {
                for(int x = 0; x < width; ++x) {
                    double sum_r = 0., sum_g = 0., sum_b = 0., sum_a = 0.;
                    int pixel_count = 0;
                    for(int j = -1; j <= 1; ++j) {
                        const int ref_y = std::min(std::max(0, y + j), height - 1);
                        for(int i = -1; i <= 1; ++i) {
                            const int ref_x = std::min(std::max(0, x + i), width - 1);
                            const double ref_op = op[i + 1][j + 1];
                            const gd::color color = img.pixel_fast(ref_x, ref_y);
                            const int a = (color & 0x7f000000) >> 24;
                            if(a == 0x7f) {
                                continue;
                            }
                            if(std::abs(ref_op) > 0.0001) {
                                const int r = (color & 0xff0000) >> 16;
                                const int g = (color & 0x00ff00) >> 8;
                                const int b = (color & 0x0000ff);
                                sum_r += r * ref_op;
                                sum_g += g * ref_op;
                                sum_b += b * ref_op;
                                sum_a += a * ref_op;
                            }
                            ++pixel_count;
                        }
                    }

                    if(pixel_count > 0) {
                        const int r = std::max(0, std::min(255, static_cast<int>(sum_r / filter_div + offset + 0.5)));
                        const int g = std::max(0, std::min(255, static_cast<int>(sum_g / filter_div + offset + 0.5)));
                        const int b = std::max(0, std::min(255, static_cast<int>(sum_b / filter_div + offset + 0.5)));
                        const int a = std::max(0, std::min(127, static_cast<int>(sum_a + 0.5)));
                        dst.pixel_fast(x, y, (a << 24) | (r << 16) | (g << 8) | b);
                    } else {
                        dst.pixel_fast(x, y, 0x7fffffff);
                    }
                }
            }
            img.swap(dst);
            return true;
        }
    }

    bool fill_background(gd &img, gd::color bg) {
        const int bg_r = (bg & 0xff0000) >> 16;
        const int bg_g = (bg & 0x00ff00) >>  8;
        const int bg_b = (bg & 0x0000ff);
        img.convert_to_true_color();
        img.alpha_blending(false);
        const int width = img.width();
        const int height = img.height();
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int a = (color & 0x7f000000) >> 24;
                if(a == 0) {
                    continue;
                }
                const double alpha = static_cast<double>(127 - a) / 127.0;
                const int org_r = (color & 0xff0000) >> 16;
                const int org_g = (color & 0x00ff00) >>  8;
                const int org_b = (color & 0x0000ff);
                const int r = static_cast<int>(org_r * alpha + bg_r * (1.0 - alpha) + 0.5);
                const int g = static_cast<int>(org_g * alpha + bg_g * (1.0 - alpha) + 0.5);
                const int b = static_cast<int>(org_b * alpha + bg_b * (1.0 - alpha) + 0.5);
                img.pixel_fast(x, y, (r << 16) | (g << 8) | b);
            }
        }
        return true;
    }

    bool grayscale(gd &img) {
        img.convert_to_true_color();
        img.alpha_blending(false);
        const int width = img.width();
        const int height = img.height();
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int r = (color & 0xff0000) >> 16;
                const int g = (color & 0x00ff00) >>  8;
                const int b = (color & 0x0000ff);
                const int a = (color & 0x7f000000); // そのまま使うのでビットシフトしない
                const int gray = (77 * r + 150 * g + 29 * b + 128) / 256; // 0.298912 * r + 0.586611 * g + 0.114478 * b + 0.5
                img.pixel_fast(x, y, a | (gray * 0x010101));
            }
        }
        return true;
    }

    bool colorize(gd &img, int red, int green, int blue, int alpha) {
        img.convert_to_true_color();
        img.alpha_blending(false);
        const int width = img.width();
        const int height = img.height();
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int r = ((color & 0xff0000) >> 16) + red;
                const int g = ((color & 0x00ff00) >>  8) + green;
                const int b = (color & 0x0000ff) + blue;
                const int a = ((color & 0x7f000000) >> 24) + alpha;
                const int new_color = 
                    ((a < 0 ? 0 : a > 127 ? 127 : a) << 24) |
                    ((r < 0 ? 0 : r > 255 ? 255 : r) << 16) |
                    ((g < 0 ? 0 : g > 255 ? 255 : g) <<  8) |
                     (b < 0 ? 0 : b > 255 ? 255 : b);
                img.pixel_fast(x, y, new_color);
            }
        }
        return true;
    }

    bool binarize(gd &img, bool is_grayscaled) {
        img.convert_to_true_color();
        img.alpha_blending(false);
        int hist_r[256] = {};
        int hist_g[256] = {};
        int hist_b[256] = {};
        long total_r = 0;
        long total_g = 0;
        long total_b = 0;
        int pixel_count = 0;
        const int width = img.width();
        const int height = img.height();

        // ヒストグラムを取得
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int a = (color & 0x7f000000) >> 24;
                if(a == 0x7f) {
                    // 完全透明なのでスキップ
                    continue;
                }
                ++pixel_count;
                const int r = (color & 0xff0000) >> 16;
                ++hist_r[r];
                total_r += r;
                if(!is_grayscaled) {
                    const int g = (color & 0x00ff00) >> 8;
                    ++hist_g[g];
                    total_g += g;
                    const int b = (color & 0x0000ff);
                    ++hist_b[b];
                    total_b += b;
                }
            }
        }

        // 完全に透明な画像だったわ
        if(pixel_count < 1) {
            return true;
        }

        // 大津の手法により閾値を計算
        const int otsu_threshold_r = otsu_threshold((double)total_r / (double)pixel_count, hist_r);
        const int otsu_threshold_g = is_grayscaled ? otsu_threshold_r : otsu_threshold((double)total_g / (double)pixel_count, hist_g);
        const int otsu_threshold_b = is_grayscaled ? otsu_threshold_r : otsu_threshold((double)total_b / (double)pixel_count, hist_b);

        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int a = (color & 0x7f000000);
                const int r = ((color & 0x00ff0000) >> 16) <= otsu_threshold_r ? 0x00 : 0xff;
                const int g = ((color & 0x0000ff00) >> 8)  <= otsu_threshold_g ? 0x00 : 0xff;
                const int b = ((color & 0x000000ff))       <= otsu_threshold_b ? 0x00 : 0xff;
                img.pixel_fast(x, y, a | (r << 16) | (g << 8) | b);
            }
        }
        return true;
    }

    bool negate(gd &img) {
        img.convert_to_true_color();
        img.alpha_blending(false);
        const int width = img.width();
        const int height = img.height();
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int a = (color & 0x7f000000);
                const int r = 255 - ((color & 0x00ff0000) >> 16);
                const int g = 255 - ((color & 0x0000ff00) >> 8);
                const int b = 255 - ((color & 0x000000ff));
                img.pixel_fast(x, y, a | (r << 16) | (g << 8) | b);
            }
        }
        return true;
    }

    bool pixelate(gd &img, int size) {
        if(size < 1) {
            std::cerr << "モザイクのサイズは 1 以上である必要があります" << std::endl;
            return false;
        }
        img.convert_to_true_color();
        const int width = img.width();
        const int height = img.height();
        const int blocks_x = (width + size - 1) / size;
        const int blocks_y = (height + size - 1) / size;
        gd out(width, height);
        out.alpha_blending(false);
        for(int block_y = 0; block_y < blocks_y; ++block_y) {
            const int min_y = block_y * size;
            const int max_y = std::min((block_y + 1) * size, height);
            for(int block_x = 0; block_x < blocks_x; ++block_x) {
                const int min_x = block_x * size;
                const int max_x = std::min((block_x + 1) * size, width);
                int total_r = 0, total_g = 0, total_b = 0, total_a = 0;
                int pixel_count = 0;
                for(int y = min_y; y < max_y; ++y) {
                    for(int x = min_x; x < max_x; ++x) {
                        const gd::color color = img.pixel_fast(x, y);
                        const int a = (color & 0x7f000000) >> 24;
                        if(a == 0x7f) {
                            // 完全透明
                            continue;
                        }
                        const int r = (color & 0xff0000) >> 16;
                        const int g = (color & 0x00ff00) >> 8;
                        const int b = (color & 0x0000ff);
                        total_r += r;
                        total_g += g;
                        total_b += b;
                        total_a += a;
                        ++pixel_count;
                    }
                }
                const int fill_color =
                    (pixel_count < 1)
                        ? 0x7fffffff
                        : ((static_cast<int>((double)total_a / (double)pixel_count + 0.5) << 24) |
                           (static_cast<int>((double)total_r / (double)pixel_count + 0.5) << 16) |
                           (static_cast<int>((double)total_g / (double)pixel_count + 0.5) <<  8) |
                           (static_cast<int>((double)total_b / (double)pixel_count + 0.5)));
                out.fill_rect(min_x, min_y, max_x, max_y, fill_color);
            }
        }
        img.swap(out);
        return true;
    }

    bool emboss(gd &img) {
        const double filter[3][3] = {
            { 1.5, 0.0, 0.0},
            { 0.0, 0.0, 0.0},
            { 0.0, 0.0,-1.5}
        };
        return fill_background(img, 0xffffff) && grayscale(img) && apply_operator_3x3(img, filter, 1, 127);
    }

    bool gaussian_blur(gd &img) {
        const double filter[3][3] = {
            { 1./16., 2./16., 1./16. },
            { 2./16., 4./16., 2./16. },
            { 1./16., 2./16., 1./16. }
        };
        return apply_operator_3x3(img, filter, 1, 0);
    }

    bool rotate_fast(gd &img, int degree) {
        degree %= 360;
        assert(degree % 90 == 0);
        if(degree == 0) {
            return true;
        }
        const int width = img.width();
        const int height = img.height();
        gd dst(height, width);
        dst.alpha(false, true);
        dst.copy_rotated(img, height / 2.0, width / 2.0, 0, 0, width, height, degree);
        img.swap(dst);
        return true;
    }
}
