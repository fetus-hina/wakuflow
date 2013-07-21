#pragma once
#ifndef MANIP_H_
#define MANIP_H_
class gd;
namespace manip {
    enum GRAYSCALE_METHOD {
        GRAY_AVERAGE,
        GRAY_NTSC,
    };
    bool grayscale(gd &, GRAYSCALE_METHOD);
}
#endif
