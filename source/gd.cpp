#include <cassert>
#include <boost/swap.hpp>
#include "gd.h"

#define VALIDATE_IMAGE() do { if(!m_image) throw gd_exception("image not ready"); } while(false)
#define VALIDATE_BINARY_RANGE(begin, end) do { assert((begin)), assert((end)),  assert((end) - (begin) > 0); } while(false)

gd::gd() : m_image(NULL) {
}

gd::gd(int w, int h) : m_image(NULL) {
    create_true_color(w, h);
}

gd::gd(const char *begin, const char *end) : m_image(NULL) {
    load_unknown_binary(begin, end);
}

gd::~gd() {
    destroy();
}

void gd::destroy() {
    if(!m_image) {
        return;
    }
    ::gdImageDestroy(m_image);
    m_image = NULL;
}

void gd::create_true_color(int w, int h) {
    destroy();
    m_image = ::gdImageCreateTrueColor(w, h);
    if(!m_image) {
        throw gd_exception("Could not create new image");
    }
}

void gd::load_unknown_binary(const char *begin, const char *end) {
    VALIDATE_BINARY_RANGE(begin, end);
    destroy();
    if(!try_load_png_binary(begin, end) &&
       !try_load_jpeg_binary(begin, end) &&
       !try_load_gif_binary(begin, end))
    {
        throw gd_exception("Could not load binary data");
    }
}

void gd::load_png_binary(const char *begin, const char *end) {
    VALIDATE_BINARY_RANGE(begin, end);
    destroy();
    if(!try_load_png_binary(begin, end)) {
        throw gd_exception("Could not load png binary data");
    }
}

void gd::convert_to_true_color() {
    VALIDATE_IMAGE();
    if(is_true_color()) {
        return;
    }
    const int w = width();
    const int h = height();
    gd new_image(w, h);
    new_image.alpha_blending(false);
    new_image.fill(0, 0, new_image.color_allocate(0xff, 0xff, 0xff, 0x7f));
    new_image.alpha_blending(true);
    new_image.save_alpha(false);
    new_image.copy_merge(*this, 0, 0, 0, 0, w, h, 100);
    swap(new_image);
}

void gd::swap(gd &far) {
    boost::swap(m_image, far.m_image);
}

void gd::copy(const gd &src, int dx, int dy, int sx, int sy, int w, int h) {
    VALIDATE_IMAGE();
    ::gdImageCopy(m_image, src.m_image, dx, dy, sx, sy, w, h);
}

void gd::copy_merge(const gd &src, int dstX, int dstY, int srcX, int srcY, int w, int h, int pct) {
    VALIDATE_IMAGE();
    ::gdImageCopyMerge(m_image, src.m_image, dstX, dstY, srcX, srcY, w, h, pct);
}

void gd::copy_resize(const gd &src, int dx, int dy, int sx, int sy, int dw, int dh, int sw, int sh) {
    VALIDATE_IMAGE();
    ::gdImageCopyResampled(m_image, src.m_image, dx, dy, sx, sy, dw, dh, sw, sh);
}

void gd::fill(int x, int y, color c) {
    VALIDATE_IMAGE();
    ::gdImageFill(m_image, x, y, c);
}

void gd::fill_rect(int x1, int y1, int x2, int y2, color c) {
    VALIDATE_IMAGE();
    ::gdImageFilledRectangle(m_image, x1, y1, x2, y2, c);
}

void gd::resize_fit(int target_w, int target_h) {
    VALIDATE_IMAGE();
    assert(target_w > 0);
    assert(target_h > 0);
    const int my_w = width();
    const int my_h = height();
    const double rate = std::max(1.0 * target_w / my_w, 1.0 * target_h / my_h);
    const int tmp_w = static_cast<int>(my_w * rate + 0.5);
    const int tmp_h = static_cast<int>(my_h * rate + 0.5);
    
    gd tmp(tmp_w, tmp_h);
    tmp.alpha_blending(false);
    tmp.fill(0, 0, tmp.color_allocate(0xff, 0xff, 0xff, 0x7f));
    tmp.alpha_blending(true);
    tmp.save_alpha(false);
    tmp.copy_resize(*this, 0, 0, 0, 0, tmp_w, tmp_h, my_w, my_h);

    const int crop_x = static_cast<int>(tmp_w / 2.0 - target_w / 2.0 + 0.5);
    const int crop_y = static_cast<int>(tmp_h / 2.0 - target_h / 2.0 + 0.5);
    gd dst(target_w, target_h);
    dst.alpha_blending(false);
    dst.fill(0, 0, dst.color_allocate(0xff, 0xff, 0xff, 0x7f));
    dst.alpha_blending(true);
    dst.save_alpha(false);
    dst.copy_merge(tmp, 0, 0, crop_x, crop_y, target_w, target_h, 100);

    swap(dst);

    assert(width() == target_w);
    assert(height() == target_h);
}

gd::color gd::color_allocate(uint8_t r, uint8_t g, uint8_t b) {
    VALIDATE_IMAGE();
    return ::gdImageColorAllocate(m_image, r, g, b);
}

gd::color gd::color_allocate(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    VALIDATE_IMAGE();
    return ::gdImageColorAllocateAlpha(m_image, r, g, b, a);
}

void gd::alpha_blending(bool mode) {
    VALIDATE_IMAGE();
    return ::gdImageAlphaBlending(m_image, mode ? 1 : 0);
}

void gd::save_alpha(bool mode) {
    VALIDATE_IMAGE();
    return ::gdImageSaveAlpha(m_image, mode ? 1 : 0);
}

int gd::width() const {
    VALIDATE_IMAGE();
    return gdImageSX(m_image);
}

int gd::height() const {
    VALIDATE_IMAGE();
    return gdImageSY(m_image);
}

bool gd::is_true_color() const {
    VALIDATE_IMAGE();
    return static_cast<bool>(gdImageTrueColor(m_image));
}

bool gd::is_palette() const {
    return !is_true_color();
}

void gd::save_png(std::vector<char> &out, int level) const {
    VALIDATE_IMAGE();
    int size = 0;
    void *bin = gdImagePngPtrEx(m_image, &size, level);
    out.assign(static_cast<char*>(bin), static_cast<char*>(bin) + size);
    gdFree(bin);
}

bool gd::try_load_png_binary(const char *begin, const char *end) {
    assert(m_image == NULL);
    VALIDATE_BINARY_RANGE(begin, end);
    m_image = ::gdImageCreateFromPngPtr(static_cast<int>(end - begin), const_cast<char *>(begin));
    return !!m_image;
}

bool gd::try_load_jpeg_binary(const char *begin, const char *end) {
    assert(m_image == NULL);
    VALIDATE_BINARY_RANGE(begin, end);
    m_image = ::gdImageCreateFromJpegPtr(static_cast<int>(end - begin), const_cast<char *>(begin));
    return !!m_image;
}

bool gd::try_load_gif_binary(const char *begin, const char *end) {
    assert(m_image == NULL);
    VALIDATE_BINARY_RANGE(begin, end);
    m_image = ::gdImageCreateFromGifPtr(static_cast<int>(end - begin), const_cast<char *>(begin));
    return !!m_image;
}
