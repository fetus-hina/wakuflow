#pragma once
#ifndef GD_H_
#define GD_H_
#include <stdexcept>
#include <string>
#include <vector>
#include <stdint.h>
#include <gd.h>
#include <boost/noncopyable.hpp>

class gd_exception : public std::runtime_error {
public:
    gd_exception(const std::string &msg) : runtime_error(msg) { }
};

class gd : boost::noncopyable {
public:
    typedef gd self_type;
    typedef int color;

    gd();
    gd(int sx, int sy);
    gd(const char *begin, const char *end);
    ~gd();

    void destroy();
    void create_true_color(int w, int h);
    void load_unknown_binary(const char *begin, const char *end);
    void load_png_binary(const char *begin, const char *end);
    void swap(gd &);
    void convert_to_true_color();

    void copy(const gd &src, int dstX, int dstY, int srcX, int srcY, int w, int h);
    void copy_merge(const gd &src, int dstX, int dstY, int srcX, int srcY, int w, int h, int pct);
    void copy_resize(const gd &src, int dx, int dy, int sx, int sy, int dw, int dh, int sw, int sh);
    void fill(int x, int y, color c);
    void fill_rect(int x1, int y1, int x2, int y2, color c);
    void resize_fit(int w, int h);

    color color_allocate(uint8_t r, uint8_t g, uint8_t b);
    color color_allocate(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    void alpha_blending(bool mode);
    void save_alpha(bool mode);
    void alpha(bool blend, bool save);

    int width() const;
    int height() const;
    bool is_true_color() const;
    bool is_palette() const;

    void save_png(std::vector<char> &out, int level = -1) const;

private:
    bool try_load_png_binary(const char *begin, const char *end);
    bool try_load_jpeg_binary(const char *begin, const char *end);
    bool try_load_gif_binary(const char *begin, const char *end);

    gdImage *m_image;
};

inline void gd::alpha(bool blend, bool save) {
    alpha_blending(blend);
    save_alpha(save);
}

#endif
