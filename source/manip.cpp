#include "manip.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <utility>
#include <algorithm>
#include <boost/multi_array.hpp>
#include <boost/bind.hpp>
#include "gd.h"
#include "../image/image.h"

#define GOSA_KAKUSAN_YCBCR(dx, dy, rate, rate_total) do { \
        const int tx = x + d * dx; \
        const int ty = y + dy; \
        if(0 <= tx && tx < width && 0 <= ty && ty < height) { \
            gosa[tx][ty][0] += gosa_y  * rate / rate_total; \
            gosa[tx][ty][1] += gosa_cb * rate / rate_total; \
            gosa[tx][ty][2] += gosa_cr * rate / rate_total; \
        } \
    } while(0)

#define GOSA_KAKUSAN_RGB(dx, dy, rate, rate_total) do { \
        const int tx = x + d * dx; \
        const int ty = y + dy; \
        if(0 <= tx && tx < width && 0 <= ty && ty < height) { \
            gosa[tx][ty][0] += gosa_r * rate / rate_total; \
            gosa[tx][ty][1] += gosa_g * rate / rate_total; \
            gosa[tx][ty][2] += gosa_b * rate / rate_total; \
        } \
    } while(0)

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
                    for(int j = -1; j <= 1; ++j) {
                        const int ref_y = std::min(std::max(0, y + j), height - 1);
                        for(int i = -1; i <= 1; ++i) {
                            const int ref_x = std::min(std::max(0, x + i), width - 1);
                            const double ref_op = op[i + 1][j + 1];
                            const gd::color color = img.pixel_fast(ref_x, ref_y);
                            const int a = (color & 0x7f000000) >> 24;
                            if(a == 0x7f) {
                                sum_a += a * ref_op;
                            } else if(std::abs(ref_op) > 0) {
                                const int r = (color & 0xff0000) >> 16;
                                const int g = (color & 0x00ff00) >> 8;
                                const int b = (color & 0x0000ff);
                                sum_r += r * ref_op;
                                sum_g += g * ref_op;
                                sum_b += b * ref_op;
                                sum_a += a * ref_op;
                            }
                        }
                    }

                    const int r = std::max(0, std::min(255, static_cast<int>(sum_r / filter_div + offset + 0.5)));
                    const int g = std::max(0, std::min(255, static_cast<int>(sum_g / filter_div + offset + 0.5)));
                    const int b = std::max(0, std::min(255, static_cast<int>(sum_b / filter_div + offset + 0.5)));
                    const int a = std::max(0, std::min(127, static_cast<int>(sum_a + 0.5)));
                    dst.pixel_fast(x, y, (a << 24) | (r << 16) | (g << 8) | b);
                }
            }
            img.swap(dst);
            return true;
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

        bool do_grayscale(gd &img, int black = 0, int white = 255, gd::color background = 0x7fffffff) {
            img.convert_to_true_color();
            img.alpha_blending(false);
            if((background & 0x7f000000) != 0x7f000000) {
                if(!fill_background(img, background)) {
                    return false;
                }
            }
            const int width = img.width();
            const int height = img.height();
            const double range = white - black;
            for(int y = 0; y < height; ++y) {
                for(int x = 0; x < width; ++x) {
                    const gd::color color = img.pixel_fast(x, y);
                    const int r = (color & 0xff0000) >> 16;
                    const int g = (color & 0x00ff00) >>  8;
                    const int b = (color & 0x0000ff);
                    const int a = (color & 0x7f000000); // そのまま使うのでビットシフトしない
                    const double gray = (r * 0.298912) + (g * 0.586611) + (b * 0.114478);
                    const int put =
                        (gray <= black)
                            ? 0
                            : (gray >= white)
                                ? 255
                                : std::min(255, std::max(0, static_cast<int>((gray - black) * 255. / range + 0.5)));
                    img.pixel_fast(x, y, a | (put * 0x010101));
                }
            }
            return true;
        }

        inline int convert_to_websafe_color(int c) {
            return c < 0 ? 0 : c > 255 ? 255 : ((int)round(c / 51.)) * 51;
        }

        inline double rgb_to_y(int r, int g, int b) {
            return (0.29900 * r) + (0.58700 * g) + (0.11400 * b);
        }

        inline double rgb_to_cb(int r, int g, int b) {
            return (-0.16874 * r) - (0.33126 * g) + (0.50000 * b) + 128.;
        }

        inline double rgb_to_cr(int r, int g, int b) {
            return (0.50000 * r) - (0.41869 * g) - (0.08131 * b) + 128.;
        }

        inline int ycbcr_to_rgb(double y, double cb, double cr) {
            cb -= 128.;
            cr -= 128.;
            const double r_ = y                  + (1.40200 * cr);
            const double g_ = y - (0.34414 * cb) - (0.71414 * cr);
            const double b_ = y + (1.77200 * cb);
            const int r = static_cast<int>(r_ < 0.5 ? 0 : r_ >= 254.5 ? 255.0 : (r_ + 0.5));
            const int g = static_cast<int>(g_ < 0.5 ? 0 : g_ >= 254.5 ? 255.0 : (g_ + 0.5));
            const int b = static_cast<int>(b_ < 0.5 ? 0 : b_ >= 254.5 ? 255.0 : (b_ + 0.5));
            return (r << 16) | (g << 8) | b;
        }

        inline size_t find_nearest_color_y_cb_cr(double y, double cb, double cr, const int palette[], const size_t palette_size) {
            y  = y  < 0.0 ? 0.0 : y  > 255.0 ? 255.0 : y;
            cb = cb < 0.0 ? 0.0 : cb > 255.0 ? 255.0 : cb;
            cr = cr < 0.0 ? 0.0 : cr > 255.0 ? 255.0 : cr;
            size_t min_index = 0;
            double min_score = INFINITY;
            for(size_t i = 0; i < palette_size; ++i) {
                const int p_color = palette[i];
                const double p_y  = (double)((p_color & 0xff0000) >> 16);
                const double p_cb = (double)((p_color & 0x00ff00) >>  8);
                const double p_cr = (double)((p_color & 0x0000ff)      );
                const double d_cbcr = sqrt(pow(cb - p_cb, 2.) + pow(cr - p_cr, 2.));
                const double score = sqrt(pow(y - p_y, 2.) + pow(d_cbcr, 2));
                if(score < min_score) {
                    min_index = i;
                    min_score = score;
                }
            }
            return min_index;
        }

        int famicom_convert(gd &img, DITHERING_METHOD dither, const int palette[], const size_t palette_size, int used_count[]) {
            img.alpha_blending(false);
            const int width = img.width();
            const int height = img.height();
            boost::multi_array<double, 3> gosa(boost::extents[width][height][3]);
            std::fill(gosa.origin(), gosa.origin() + gosa.size(), 0);

            for(int y = 0; y < height; ++y) {
                const int d = (y % 2 == 0) ? 1 : -1;
                const int x_begin = (y % 2 == 0) ? 0 : width - 1;
                const int x_end   = (y % 2 == 0) ? width : -1;
                for(int x = x_begin; x != x_end; x += d) {
                    const gd::color color = img.pixel_fast(x, y);
                    const int r = (color & 0x00ff0000) >> 16;
                    const int g = (color & 0x0000ff00) >> 8;
                    const int b = (color & 0x000000ff);
                    const double cy = rgb_to_y(r, g, b)  + gosa[x][y][0];
                    const double cb = rgb_to_cb(r, g, b) + gosa[x][y][1];
                    const double cr = rgb_to_cr(r, g, b) + gosa[x][y][2];
                    const size_t index = find_nearest_color_y_cb_cr(cy, cb, cr, palette, palette_size);
                    ++used_count[index];
                    const int p_y  = (palette[index] & 0xff0000) >> 16;
                    const int p_cb = (palette[index] & 0x00ff00) >>  8;
                    const int p_cr = (palette[index] & 0x0000ff);
                    img.pixel_fast(x, y, ycbcr_to_rgb(p_y, p_cb, p_cr));
                    const double gosa_y  = cy - p_y;
                    const double gosa_cb = cb - p_cb;
                    const double gosa_cr = cr - p_cr;

                    switch(dither) {
                    case DITHERING_NONE:
                    default:
                        break;
                    case DITHERING_FLOYD_STEINBERG:
                        // - X 7
                        // 3 5 1
                        GOSA_KAKUSAN_YCBCR( 1, 0, 7., 16.);
                        GOSA_KAKUSAN_YCBCR(-1, 0, 3., 16.);
                        GOSA_KAKUSAN_YCBCR( 0, 0, 5., 16.);
                        GOSA_KAKUSAN_YCBCR( 1, 0, 1., 16.);
                        break;
                    case DITHERING_SIERRA_3LINE:
                        // - - X 5 3
                        // 2 4 5 4 2
                        // 0 2 3 2 0
                        GOSA_KAKUSAN_YCBCR( 1, 0, 5., 32.);
                        GOSA_KAKUSAN_YCBCR( 2, 0, 3., 32.);
                        GOSA_KAKUSAN_YCBCR(-2, 1, 2., 32.);
                        GOSA_KAKUSAN_YCBCR(-1, 1, 4., 32.);
                        GOSA_KAKUSAN_YCBCR( 0, 1, 5., 32.);
                        GOSA_KAKUSAN_YCBCR( 1, 1, 4., 32.);
                        GOSA_KAKUSAN_YCBCR( 2, 1, 2., 32.);
                        GOSA_KAKUSAN_YCBCR(-1, 2, 2., 32.);
                        GOSA_KAKUSAN_YCBCR( 0, 2, 3., 32.);
                        GOSA_KAKUSAN_YCBCR( 1, 2, 2., 32.);
                        break;
                    case DITHERING_SIERRA_2LINE:
                        // - - X 4 3
                        // 1 2 3 2 1
                        GOSA_KAKUSAN_YCBCR( 1, 0, 4., 16.);
                        GOSA_KAKUSAN_YCBCR( 2, 0, 3., 16.);
                        GOSA_KAKUSAN_YCBCR(-2, 1, 1., 16.);
                        GOSA_KAKUSAN_YCBCR(-1, 1, 2., 16.);
                        GOSA_KAKUSAN_YCBCR( 0, 1, 3., 16.);
                        GOSA_KAKUSAN_YCBCR( 1, 1, 2., 16.);
                        GOSA_KAKUSAN_YCBCR( 2, 1, 1., 16.);
                        break;
                    case DITHERING_SIERRA_LITE:
                        // - X 2
                        // 1 1 0
                        GOSA_KAKUSAN_YCBCR( 1, 0, 2., 4.);
                        GOSA_KAKUSAN_YCBCR(-1, 1, 1., 4.);
                        GOSA_KAKUSAN_YCBCR( 0, 1, 1., 4.);
                        break;
                    case DITHERING_ATKINSON:
                        // - - X 1 1
                        // 0 1 1 1 0
                        // 0 0 1 0 0 ※合計6だが8で割る(75%拡散)
                        GOSA_KAKUSAN_YCBCR( 1, 0, 1., 8.);
                        GOSA_KAKUSAN_YCBCR( 2, 0, 1., 8.);
                        GOSA_KAKUSAN_YCBCR(-1, 1, 1., 8.);
                        GOSA_KAKUSAN_YCBCR( 0, 1, 1., 8.);
                        GOSA_KAKUSAN_YCBCR( 1, 1, 1., 8.);
                        GOSA_KAKUSAN_YCBCR( 0, 2, 1., 8.);
                        break;
                    }
                }
            }
            int count = 0;
            for(size_t i = 0; i < palette_size; ++i) {
                if(used_count[i] > 0) {
                    ++count;
                }
            }
            return count;
        }
    }

    bool grayscale(gd &img) {
        return do_grayscale(img, 0x00, 0xff, (0x7f << 24));
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

    bool binarize(gd &img, bool is_grayscaled, THRESHOLDING thresholding, DITHERING_METHOD dithering) {
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

        boost::multi_array<double, 3> gosa(boost::extents[width][height][3]);
        std::fill(gosa.origin(), gosa.origin() + gosa.size(), 0);

        // 閾値を計算
        const int threshold_r = (thresholding == THRESHOLD_HALF) ? 128 : otsu_threshold((double)total_r / (double)pixel_count, hist_r);
        const int threshold_g = is_grayscaled ? threshold_r : (thresholding == THRESHOLD_HALF) ? 128 : otsu_threshold((double)total_g / (double)pixel_count, hist_g);
        const int threshold_b = is_grayscaled ? threshold_r : (thresholding == THRESHOLD_HALF) ? 128 : otsu_threshold((double)total_b / (double)pixel_count, hist_b);
        for(int y = 0; y < height; ++y) {
            const int d = (y % 2 == 0) ? 1 : -1;
            const int x_begin = (y % 2 == 0) ? 0 : width - 1;
            const int x_end   = (y % 2 == 0) ? width : -1;
            for(int x = x_begin; x != x_end; x += d) {
                const gd::color color = img.pixel_fast(x, y);
                const int a = (color & 0x7f000000);
                const double r = static_cast<double>((color & 0xff0000) >> 16) + gosa[x][y][0];
                const double g = static_cast<double>((color & 0x00ff00) >>  8) + gosa[x][y][1];
                const double b = static_cast<double>((color & 0x0000ff)      ) + gosa[x][y][2];
                const int put_r = (r < threshold_r) ? 0x00 : 0xff;
                const int put_g = (g < threshold_g) ? 0x00 : 0xff;
                const int put_b = (b < threshold_b) ? 0x00 : 0xff;
                img.pixel_fast(x, y, a | (put_r << 16) | (put_g << 8) | put_b);
                if(a < 0x7f000000) {
                    const double gosa_r = r - static_cast<double>(put_r);
                    const double gosa_g = g - static_cast<double>(put_g);
                    const double gosa_b = b - static_cast<double>(put_b);
                    switch(dithering) {
                    case DITHERING_NONE:
                    default:
                        break;
                    case DITHERING_FLOYD_STEINBERG:
                        // - X 7
                        // 3 5 1
                        GOSA_KAKUSAN_RGB( 1, 0, 7., 16.);
                        GOSA_KAKUSAN_RGB(-1, 0, 3., 16.);
                        GOSA_KAKUSAN_RGB( 0, 0, 5., 16.);
                        GOSA_KAKUSAN_RGB( 1, 0, 1., 16.);
                        break;
                    case DITHERING_SIERRA_3LINE:
                        // - - X 5 3
                        // 2 4 5 4 2
                        // 0 2 3 2 0
                        GOSA_KAKUSAN_RGB( 1, 0, 5., 32.);
                        GOSA_KAKUSAN_RGB( 2, 0, 3., 32.);
                        GOSA_KAKUSAN_RGB(-2, 1, 2., 32.);
                        GOSA_KAKUSAN_RGB(-1, 1, 4., 32.);
                        GOSA_KAKUSAN_RGB( 0, 1, 5., 32.);
                        GOSA_KAKUSAN_RGB( 1, 1, 4., 32.);
                        GOSA_KAKUSAN_RGB( 2, 1, 2., 32.);
                        GOSA_KAKUSAN_RGB(-1, 2, 2., 32.);
                        GOSA_KAKUSAN_RGB( 0, 2, 3., 32.);
                        GOSA_KAKUSAN_RGB( 1, 2, 2., 32.);
                        break;
                    case DITHERING_SIERRA_2LINE:
                        // - - X 4 3
                        // 1 2 3 2 1
                        GOSA_KAKUSAN_RGB( 1, 0, 4., 16.);
                        GOSA_KAKUSAN_RGB( 2, 0, 3., 16.);
                        GOSA_KAKUSAN_RGB(-2, 1, 1., 16.);
                        GOSA_KAKUSAN_RGB(-1, 1, 2., 16.);
                        GOSA_KAKUSAN_RGB( 0, 1, 3., 16.);
                        GOSA_KAKUSAN_RGB( 1, 1, 2., 16.);
                        GOSA_KAKUSAN_RGB( 2, 1, 1., 16.);
                        break;
                    case DITHERING_SIERRA_LITE:
                        // - X 2
                        // 1 1 0
                        GOSA_KAKUSAN_RGB( 1, 0, 2., 4.);
                        GOSA_KAKUSAN_RGB(-1, 1, 1., 4.);
                        GOSA_KAKUSAN_RGB( 0, 1, 1., 4.);
                        break;
                    case DITHERING_ATKINSON:
                        // - - X 1 1
                        // 0 1 1 1 0
                        // 0 0 1 0 0 ※合計6だが8で割る(75%拡散)
                        GOSA_KAKUSAN_RGB( 1, 0, 1., 8.);
                        GOSA_KAKUSAN_RGB( 2, 0, 1., 8.);
                        GOSA_KAKUSAN_RGB(-1, 1, 1., 8.);
                        GOSA_KAKUSAN_RGB( 0, 1, 1., 8.);
                        GOSA_KAKUSAN_RGB( 1, 1, 1., 8.);
                        GOSA_KAKUSAN_RGB( 0, 2, 1., 8.);
                        break;
                    }
                }
            }
        }
        return true;
    }

    bool websafe(gd &img, DITHERING_METHOD dither) {
        img.convert_to_true_color();
        img.alpha_blending(false);
        const int width = img.width();
        const int height = img.height();
        boost::multi_array<double, 3> gosa(boost::extents[width][height][3]);
        std::fill(gosa.origin(), gosa.origin() + gosa.size(), 0); // 要らない？

        for(int y = 0; y < height; ++y) {
            const int d = (y % 2 == 0) ? 1 : -1;
            const int x_begin = (y % 2 == 0) ? 0 : width - 1;
            const int x_end   = (y % 2 == 0) ? width : -1;
            for(int x = x_begin; x != x_end; x += d) {
                const gd::color color = img.pixel_fast(x, y);
                const int a = (color & 0x7f000000);
                const double r = static_cast<double>((color & 0x00ff0000) >> 16) + gosa[x][y][0];
                const double g = static_cast<double>((color & 0x0000ff00) >>  8) + gosa[x][y][1];
                const double b = static_cast<double>((color & 0x000000ff)      ) + gosa[x][y][2];
                const int web_r = convert_to_websafe_color(static_cast<int>(round(r)));
                const int web_g = convert_to_websafe_color(static_cast<int>(round(g)));
                const int web_b = convert_to_websafe_color(static_cast<int>(round(b)));
                if(a < 0x7f000000) {
                    const double gosa_r = r - static_cast<double>(web_r);
                    const double gosa_g = g - static_cast<double>(web_g);
                    const double gosa_b = b - static_cast<double>(web_b);
                    switch(dither) {
                    case DITHERING_NONE:
                    default:
                        break;
                    case DITHERING_FLOYD_STEINBERG:
                        // - X 7
                        // 3 5 1
                        GOSA_KAKUSAN_RGB( 1, 0, 7., 16.);
                        GOSA_KAKUSAN_RGB(-1, 0, 3., 16.);
                        GOSA_KAKUSAN_RGB( 0, 0, 5., 16.);
                        GOSA_KAKUSAN_RGB( 1, 0, 1., 16.);
                        break;
                    case DITHERING_SIERRA_3LINE:
                        // - - X 5 3
                        // 2 4 5 4 2
                        // 0 2 3 2 0
                        GOSA_KAKUSAN_RGB( 1, 0, 5., 32.);
                        GOSA_KAKUSAN_RGB( 2, 0, 3., 32.);
                        GOSA_KAKUSAN_RGB(-2, 1, 2., 32.);
                        GOSA_KAKUSAN_RGB(-1, 1, 4., 32.);
                        GOSA_KAKUSAN_RGB( 0, 1, 5., 32.);
                        GOSA_KAKUSAN_RGB( 1, 1, 4., 32.);
                        GOSA_KAKUSAN_RGB( 2, 1, 2., 32.);
                        GOSA_KAKUSAN_RGB(-1, 2, 2., 32.);
                        GOSA_KAKUSAN_RGB( 0, 2, 3., 32.);
                        GOSA_KAKUSAN_RGB( 1, 2, 2., 32.);
                        break;
                    case DITHERING_SIERRA_2LINE:
                        // - - X 4 3
                        // 1 2 3 2 1
                        GOSA_KAKUSAN_RGB( 1, 0, 4., 16.);
                        GOSA_KAKUSAN_RGB( 2, 0, 3., 16.);
                        GOSA_KAKUSAN_RGB(-2, 1, 1., 16.);
                        GOSA_KAKUSAN_RGB(-1, 1, 2., 16.);
                        GOSA_KAKUSAN_RGB( 0, 1, 3., 16.);
                        GOSA_KAKUSAN_RGB( 1, 1, 2., 16.);
                        GOSA_KAKUSAN_RGB( 2, 1, 1., 16.);
                        break;
                    case DITHERING_SIERRA_LITE:
                        // - X 2
                        // 1 1 0
                        GOSA_KAKUSAN_RGB( 1, 0, 2., 4.);
                        GOSA_KAKUSAN_RGB(-1, 1, 1., 4.);
                        GOSA_KAKUSAN_RGB( 0, 1, 1., 4.);
                        break;
                    case DITHERING_ATKINSON:
                        // - - X 1 1
                        // 0 1 1 1 0
                        // 0 0 1 0 0 ※合計6だが8で割る(75%拡散)
                        GOSA_KAKUSAN_RGB( 1, 0, 1., 8.);
                        GOSA_KAKUSAN_RGB( 2, 0, 1., 8.);
                        GOSA_KAKUSAN_RGB(-1, 1, 1., 8.);
                        GOSA_KAKUSAN_RGB( 0, 1, 1., 8.);
                        GOSA_KAKUSAN_RGB( 1, 1, 1., 8.);
                        GOSA_KAKUSAN_RGB( 0, 2, 1., 8.);
                        break;
                    }
                }
                img.pixel_fast(x, y, a | (web_r << 16) | (web_g << 8) | web_b);
            }
        }
        return true;
    }

    bool famicom(gd &img, DITHERING_METHOD dither) {
        const int fixed_palette_y_cb_cr[] = {
            0x008080, 0x1bde71, 0x24fe6c, 0x32975e, 0x3462bc, 0x34635b, 0x346394, 0x3774d1,
            0x3c5ecd, 0x3d5d54, 0x3fa9bf, 0x46584e, 0x47c582, 0x5185eb, 0x57df47, 0x5ec1db,
            0x63483a, 0x63973c, 0x6ad43a, 0x6b43e5, 0x6bd683, 0x6c4333, 0x6c6a34, 0x7c3aa2,
            0x7c43ca, 0x7c8080, 0x808080, 0x93bf67, 0x9485cb, 0x98ba86, 0x9d5ac3, 0xa5534b,
            0xa5b53b, 0xa79f0d, 0xb342b5, 0xb4aab6, 0xb619af, 0xb8aa02, 0xc26b38, 0xc28080,
            0xc680a8, 0xc7a07b, 0xcc1b72, 0xd09a8b, 0xda4a98, 0xda959b, 0xdb6a93, 0xdb975e,
            0xe3457a, 0xe36b65, 0xe75f93, 0xe87b63, 0xed8a8d, 0xff8080, 
        };
        const size_t fixed_palette_count = sizeof(fixed_palette_y_cb_cr) / sizeof(fixed_palette_y_cb_cr[0]);

        if(!fill_background(img, 0xffffff)) {
            return false;
        }
        // それっぽさを出すために解像度を落とす
        img.resize_fit((img.width() + 1) / 2, (img.height() + 1) / 2);

        // 画像を破壊しないためにコピーを作る
        gd img_tmp(img.width(), img.height());
        img_tmp.alpha(false, true);
        img_tmp.copy(img, 0, 0, 0, 0, img.width(), img.height());

        // 1 パス目: とりあえず変換する
        std::vector<int> palette_use_count(fixed_palette_count);
        const int color_count = famicom_convert(img_tmp, dither, fixed_palette_y_cb_cr, fixed_palette_count, &palette_use_count[0]);
        if(color_count <= 25) {
            // 1 パスで同時発色可能数に収まった
            img.swap(img_tmp);
        } else {
            // 2 パス目: 25 色以内に抑える
            
            // 使われた回数が多い順に並び替える
            std::vector<std::pair<size_t, int> > counts;
            for(size_t i = 0; i < fixed_palette_count; ++i) {
                counts.push_back(std::pair<size_t, int>(i, palette_use_count[i]));
            }
            std::sort(counts.begin(), counts.end(), boost::bind(&std::pair<size_t, int>::second, _1) > boost::bind(&std::pair<size_t, int>::second, _2));

            // 多い方から 25 色取得する
            int palette[25] = {};
            {
                size_t i;
                std::vector<std::pair<size_t, int> >::iterator it;
                for(i = 0, it = counts.begin(); i < 25; ++i, ++it) {
                    palette[i] = fixed_palette_y_cb_cr[it->first];
                }
            }

            palette_use_count.resize(25);
            famicom_convert(img, dither, palette, 25, &palette_use_count[0]);
        }

        // 解像度を下げたので元（とほとんど同じ）サイズに変更する
        // GD の拡大が nearest neighbor なことに依存している
        img.resize_fit(img.width() * 2, img.height() * 2);
        return true;
    }

    bool gameboy(gd &img, bool scale, DITHERING_METHOD dither) {
        const int    palette_y_cb_cr[4] = { 0x3a8080, 0x6b8080, 0xb08080, 0xde8080 };
        const size_t palette_size = 4;
        int palette_use_count[4] = {};
        
        // 出力色が完全な白や黒でないので調整する
        if(!do_grayscale(img, 28, 236, 0xffffff)) {
            return false;
        }

        if(scale) {
            img.resize_fit(160, 144);
        }

        // ファミコンパレットへの変換を流用して変換する
        famicom_convert(img, dither, palette_y_cb_cr, palette_size, palette_use_count);

        // パレット置換
        const int width = img.width();
        const int height = img.height();
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int c1 = (color & 0x0000ff);
                const int c2 = (c1 < 0x52) ? 0x204631 : (c1 < 0x8d) ? 0x527f39 : (c1 < 0xc7) ? 0xaec440 : 0xd7e894;
                img.pixel_fast(x, y, c2);
            }
        }

        if(scale) {
            img.resize_fit(480, 432);
        }

        return true;
    }

    bool virtualboy(gd &img, bool scale, DITHERING_METHOD dither) {
        const int    palette_y_cb_cr[4] = { 0x008080, 0x558080, 0xaa8080, 0xff8080 };
        const size_t palette_size = 4;
        int palette_use_count[4] = {};
        
        if(!do_grayscale(img, 0, 255, 0xffffff)) {
            return false;
        }
        if(scale) {
            img.resize_fit(384, 288);   // 4:3 で切り出す
            img.resize_force(384, 224); // ピクセル数を整合する
        }
        // ファミコンパレットへの変換を流用して変換する
        famicom_convert(img, dither, palette_y_cb_cr, palette_size, palette_use_count);

        // パレット置換
        const int palette_red[4] = { 0x170515, 0x5c020a, 0xa20000, 0xe70000 };
        const int width = img.width();
        const int height = img.height();
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const gd::color color = img.pixel_fast(x, y);
                const int c1 = (color & 0x0000ff);
                const int c2 = palette_red[c1 / 85];
                img.pixel_fast(x, y, c2);
            }
        }

        if(scale) {
            img.resize_force(384, 288); // 歪ませたので戻す
        }

        gaussian_blur(img);

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

    bool sharpen(gd &img) {
        const double filter[3][3] = {
            { -1., -1., -1. },
            { -1.,  9., -1. },
            { -1., -1., -1. }
        };
        return apply_operator_3x3(img, filter, 1, 0);
    }

    bool edge(gd &img) {
        const double sobel_h[3][3] = {
            { 1, 0, -1 },
            { 2, 0, -2 },
            { 1, 0, -1 }
        };
        const double sobel_v[3][3] = {
            {  1,  2,  1 },
            {  0,  0,  0 },
            { -1, -2, -1 }
        };

        if(!fill_background(img, 0xffffff) || !grayscale(img)) {
            return false;
        }

        // ぼかす
        if(!gaussian_blur(img) || !gaussian_blur(img) || !gaussian_blur(img)) {
            return false;
        }
        const int width = img.width();
        const int height = img.height();

        // 勾配検出
        {
            gd horizontal(width, height);
            horizontal.alpha(false, false);
            horizontal.copy(img, 0, 0, 0, 0, width, height);
            if(!apply_operator_3x3(horizontal, sobel_h, 1, 127)) {
                return false;
            }
            gd vertical(width, height);
            vertical.alpha(false, false);
            vertical.copy(img, 0, 0, 0, 0, width, height);
            if(!apply_operator_3x3(vertical, sobel_v, 1, 127)) {
                return false;
            }
            for(int y = 0; y < height; ++y) {
                for(int x = 0; x < width; ++x) {
                    const double h = std::min(1.0, static_cast<double>((horizontal.pixel(x, y) & 0xff) - 127) / 127.0);
                    const double v = std::min(1.0, static_cast<double>((vertical.pixel(x, y) & 0xff) - 127) / 127.0);
                    const int edge = std::max(0, std::min(255, static_cast<int>(sqrt(h * h + v * v) * 255.0 + 0.5)));
                    const double theta = atan2(h, v) * 180.0 / M_PI;
                    const int direction_code = 
                        (theta < 22.5) ? 1 :                // 右
                        (theta < 22.5 + 45.0 * 1) ? 2 :     // 右上
                        (theta < 22.5 + 45.0 * 2) ? 3 :     // 上
                        (theta < 22.5 + 45.0 * 3) ? 4 : 5;  // 左上・左 180°を超えることはない…はず
                    img.pixel(x, y, (direction_code << 8) | edge);
                }
            }
        }

        // 細線化（と後の処理のためにヒストグラム作成）
        int hist[256] = {};
        long long sum = 0;
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const int direction_code = (img.pixel(x, y) & 0xff00) >> 8;
                const int c0 = img.pixel(x, y) & 0xff;
                int c1, c2;
                switch(direction_code) {
                case 1: case 5: // 左右
                    c1 = img.pixel_safe(x + 1, y) & 0xff;
                    c2 = img.pixel_safe(x - 1, y) & 0xff;
                    break;
                case 2: // 右上
                    c1 = img.pixel_safe(x + 1, y - 1) & 0xff;
                    c2 = img.pixel_safe(x - 1, y + 1) & 0xff;
                    break;
                case 3: // 上
                    c1 = img.pixel_safe(x, y - 1) & 0xff;
                    c2 = img.pixel_safe(x, y + 1) & 0xff;
                    break;
                case 4: // 左上
                    c1 = img.pixel_safe(x - 1, y - 1) & 0xff;
                    c2 = img.pixel_safe(x + 1, y + 1) & 0xff;
                    break;
                default:
                    assert(false);
                    return false;
                }
                if(c0 < c1 || c0 < c2) {
                    img.pixel(x, y, 0x000000);
                    ++hist[0];
                } else {
                    img.pixel(x, y, 0x010101 * c0);
                    ++hist[c0];
                    sum += c0;
                }
            }
        }

        // 閾値(HI)を計算
        const int hi_threshold = otsu_threshold((double)sum / ((double)width * (double)height), hist);

        // 閾値(LO)を計算
        sum = 0;
        for(int i = 0; i < 256; ++i) {
            if(i < hi_threshold) {
                sum += hist[i] * i;
            } else if(i == hi_threshold) {
                // nothing to do.
            } else {
                // 閾値(HI)以上の画素は全部閾値(HI)と見なす
                sum += hist[i] * hi_threshold;
                hist[hi_threshold] += hist[i];
                hist[i] = 0;
            }
        }
        const int lo_threshold = otsu_threshold((double)sum / ((double)width * (double)height), hist);

        gd dst(width, height);
        dst.alpha(false, true);
        // 閾値HI以上の画素を描画
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                dst.pixel_fast(x, y, (img.pixel_fast(x, y) & 0xff) >= hi_threshold ? 0xffffff : 0x000000);
            }
        }
        // 閾値LO以上HI未満の画素をそれなりに描画
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const int c = img.pixel_fast(x, y) & 0xff;
                if(lo_threshold <= c && c < hi_threshold) {
                    bool found = false;
                    for(int j = -1; j <= 1; ++j) {
                        for(int i = -1; i <= 1; ++i) {
                            if(i == 0 && j == 0) {
                                // 自分の画素を見ても仕方ない
                                continue;
                            }
                            // 近所にエッジは居る？
                            if((img.pixel_safe(x + i, y + j) & 0xff) >= hi_threshold) {
                                found = true;
                                break;
                            }
                        }
                        if(found) {
                            break;
                        }
                    }
                    if(found) {
                        dst.pixel_fast(x, y, 0xffffff);
                    }
                }
            }
        }

        img.swap(dst);

        return true;
    }

    bool rotate_fast(gd &img, int degree) {
        degree %= 360;
        assert(degree % 90 == 0);
        if(degree == 0) {
            return true;
        }
        const int src_width  = img.width();
        const int src_height = img.height();
        const int dst_width  = degree % 180 == 0 ? src_width : src_height;
        const int dst_height = degree % 180 == 0 ? src_height : src_width;
        gd dst(dst_width, dst_height);
        dst.alpha(false, true);
        dst.copy_rotated(img, dst_width / 2.0, dst_height / 2.0, 0, 0, src_width, src_height, degree);
        img.swap(dst);
        return true;
    }

    bool flip_horizontal(gd &img) {
        const int width = img.width();
        const int height = img.height();
        gd dst(width, height);
        dst.alpha(false, true);
        for(int x = 0; x < width; ++x) {
            const int x2 = width - x - 1;
            dst.copy(img, x2, 0, x, 0, 1, height);
        }
        img.swap(dst);
        return true;
    }

    bool flip_vertical(gd &img) {
        const int width = img.width();
        const int height = img.height();
        gd dst(width, height);
        dst.alpha(false, true);
        for(int y = 0; y < height; ++y) {
            const int y2 = height - y - 1;
            dst.copy(img, 0, y2, 0, y, width, 1);
        }
        img.swap(dst);
        return true;
    }
}
