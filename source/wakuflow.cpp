#include <stdio.h>
#include <cassert>
#include "../image/image.h"
#include "gd.h"

#include <vector>

std::vector<char> load(FILE *fp) {
    assert(fp);
    std::vector<char> data;
    char buffer[8192];
    while(!feof(fp)) {
        const size_t read_size = fread(buffer, 1, sizeof(buffer), fp);
        if(read_size > 0) {
            data.insert(data.end(), buffer, buffer + read_size);
        }
    }
    return data;
}

int main(int, char *[]) {
    // アルファブレンディングの取り扱いが全体的に意味わかんないので
    // ちゃんと調べてちゃんと実装する
    // 標準入出力はもちろん今だけ

    const std::vector<char> icon_ = load(stdin);
    gd icon(&icon_[0], &icon_[icon_.size()]);
    icon.resize_fit(174, 165);
    icon.alpha_blending(true);
    icon.save_alpha(false);

    gd waku(_binary_waku1_png_start, _binary_waku1_png_end);
    waku.convert_to_true_color();
    waku.alpha_blending(true);
    waku.save_alpha(false);

    gd built(waku.width(), waku.height());
    built.fill(0, 0, 0x00bcd77e);
    built.alpha_blending(true);
    built.save_alpha(false);
    built.copy(icon, 28, 27, 0, 0, icon.width(), icon.height());
    built.copy(waku, 0, 0, 0, 0, waku.width(), waku.height());

    built.save_alpha(true);
    std::vector<char> hoge;
    built.save_png(hoge, 9);

    fwrite(&hoge[0], hoge.size(), 1, stdout);
    return 0;
}
