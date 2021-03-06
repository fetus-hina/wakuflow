#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <errno.h>
#include "gd.h"
#include "waku.h"
#include "waku_v2.h"
#include "shuchusen.h"
#include "romanov.h"
#include "stamp.h"
#include "manip.h"
#include "animal.h"
#include "brain.h"

//FIXME
std::string config_waku_font_file;
std::string config_animal_font_file1;
std::string config_animal_font_file2;

namespace po = ::boost::program_options;

namespace {
    bool handle_program_options(int argc, char *argv[], po::variables_map &vm) {
        po::options_description opt("Options");
        opt.add_options()
            ("input,i",  po::value<std::string>(), "Input file path")
            ("output,o", po::value<std::string>(), "Output file path")
            ("font,f", po::value<std::string>(),   "TTF/OTF font path for waku v2")
            ("font2", po::value<std::string>(),    "TTF/OTF font path for animal/brain")
            ("font3", po::value<std::string>(),    "TTF/OTF font path for animal")
            ("help,h",                             "Help");

        try {
            po::store(po::parse_command_line(argc, argv, opt), vm);
            po::notify(vm);
            if(vm.count("help") != 0 || vm.count("input") != 1 || vm.count("output") != 1) {
                std::cerr << opt << std::endl;
                return false;
            }
            return true;
        } catch(std::exception &e) {
            std::cerr << opt << std::endl;
            std::cerr << e.what() << std::endl << std::endl;
            return false;
        }
    }

    bool load_input_file(const std::string &path, std::vector<char> &out) {
        std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
        if(!in) {
            return false;
        }
        char buffer[8192];
        while(true) {
            in.read(buffer, sizeof(buffer));
            out.insert(out.end(), buffer, buffer + in.gcount());
            if(in.eof()) {
                return true;
            } else if(!in) {
                return false;
            }
        }
    }

    bool save_output_file_(std::ostream &s, const std::vector<char> &out) {
        if(!s.write(&out[0], out.size())) {
            return false;
        }
        if(!s.flush()) {
            return false;
        }
        return true;
    }

    bool save_output_file(const std::string &path, const  std::vector<char> &out) {
        if(path == "-") {
            return save_output_file_(std::cout, out);
        } else {
            std::ofstream of(path.c_str(), std::ios::out | std::ios::binary);
            if(!of) {
                return false;
            }
            return save_output_file_(of, out);
        }
    }

    manip::DITHERING_METHOD dithering_method(const std::string &value, manip::DITHERING_METHOD def = manip::DITHERING_NONE) {
        if(value == "none")             { return manip::DITHERING_NONE; }
        else if(value == "floyd")       { return manip::DITHERING_FLOYD_STEINBERG; }
        else if(value == "sierra3")     { return manip::DITHERING_SIERRA_3LINE; }
        else if(value == "sierra2")     { return manip::DITHERING_SIERRA_2LINE; }
        else if(value == "sierra-lite") { return manip::DITHERING_SIERRA_LITE; }
        else if(value == "atkinson")    { return manip::DITHERING_ATKINSON; }
        std::cerr << "不明なディザリングメソッド: " << value << std::endl;
        return def;
    }

    bool execute_command(gd &image, const std::string &command, const std::vector<std::string> &options) {
        if(command == "waku1") {
            return waku::waku1(image);
        } else if(command == "waku2") {
            return waku::waku2(image);
        } else if(command == "waku3") {
            return waku::waku3(image);
        } else if(command == "waku4") {
            return waku::waku4(image);
        } else if(command == "waku_v2") {
            if(options.size() < 1) {
                std::cerr << "waku_v2 コマンドには @name の指定が必要です" << std::endl;
                return false;
            }
            return waku_v2::waku_v2(image, options[0]);
        } else if(command == "shuchusen1") {
            return shuchusen::shuchusen1(image);
        } else if(command == "shuchusen2") {
            return shuchusen::shuchusen2(image);
        } else if(command == "shuchusen3") {
            return shuchusen::shuchusen3(image);
        } else if(command == "romanov") {
            return romanov::romanov(image);
        } else if(command == "half_price") {
            return stamp::half_price(image);
        } else if(command == "kankore_half_damage") {
            return stamp::kankore_half_damage(image);
        } else if(command == "kankore_badly_damage") {
            return stamp::kankore_badly_damage(image);
        } else if(command == "grayscale") {
            return manip::grayscale(image);
        } else if(command == "sepia") {
            return manip::grayscale(image) && manip::colorize(image, 100, 50, 0, 0);
        } else if(command == "binarize") {
            const manip::THRESHOLDING threshold = (options.size() >= 1 && options[0] == "half") ? manip::THRESHOLD_HALF : manip::THRESHOLD_OTSU;
            const manip::DITHERING_METHOD method = (options.size() >= 2) ? dithering_method(options[1], manip::DITHERING_NONE) : manip::DITHERING_NONE;
            return manip::grayscale(image) && manip::binarize(image, true, threshold, method);
        } else if(command == "8colors") {
            const manip::THRESHOLDING threshold = (options.size() >= 1 && options[0] == "half") ? manip::THRESHOLD_HALF : manip::THRESHOLD_OTSU;
            const manip::DITHERING_METHOD method = (options.size() >= 2) ? dithering_method(options[1], manip::DITHERING_NONE) : manip::DITHERING_NONE;
            return manip::binarize(image, false, threshold, method);
        } else if(command == "websafe") {
            const manip::DITHERING_METHOD method = (options.size() >= 1) ? dithering_method(options[0], manip::DITHERING_NONE) : manip::DITHERING_NONE;
            return manip::websafe(image, method);
        } else if(command == "famicom") {
            const manip::DITHERING_METHOD method = (options.size() >= 1) ? dithering_method(options[0], manip::DITHERING_NONE) : manip::DITHERING_NONE;
            return manip::famicom(image, method);
        } else if(command == "gameboy") {
            const bool scale = (options.size() >= 1 && options[0] == "asis") ? false : true;
            const manip::DITHERING_METHOD method = (options.size() >= 2) ? dithering_method(options[1], manip::DITHERING_NONE) : manip::DITHERING_NONE;
            return manip::gameboy(image, scale, method);
        } else if(command == "virtualboy") {
            const bool scale = (options.size() >= 1 && options[0] == "asis") ? false : true;
            const manip::DITHERING_METHOD method = (options.size() >= 2) ? dithering_method(options[1], manip::DITHERING_NONE) : manip::DITHERING_NONE;
            return manip::virtualboy(image, scale, method);
        } else if(command == "negate") {
            return manip::negate(image);
        } else if(command == "pixelate") {
            if(options.size() < 1) {
                std::cerr << "pixleate コマンドにはサイズの指定が必要です" << std::endl;
                return false;
            }
            try {
                const int size = boost::lexical_cast<int>(options[0]);
                return manip::pixelate(image, size);
            } catch(boost::bad_lexical_cast) {
                std::cerr << "pixelate コマンドの引数は整数を設定してください" << std::endl;
                return false;
            }
        } else if(command == "emboss") {
            return manip::emboss(image);
        } else if(command == "gaussian_blur") {
            return manip::gaussian_blur(image);
        } else if(command == "sharpen") {
            return manip::sharpen(image);
        } else if(command == "edge") {
            return manip::edge(image);
        } else if(command == "rotate") {
            if(options.size() < 1) {
                std::cerr << "rotate コマンドには角度の指定が必要です" << std::endl;
                return false;
            }
            try {
                const int angle = boost::lexical_cast<int>(options[0]);
                if(angle != 0 && angle != 90 && angle != 180 && angle != 270) {
                    std::cerr << "rotate コマンドの角度は 0, 90, 180, 270 のいずれかである必要があります" << std::endl;
                    return false;
                }
                return manip::rotate_fast(image, angle); // TODO:一般化
            } catch(boost::bad_lexical_cast) {
                std::cerr << "rotate コマンドの引数は整数を設定してください" << std::endl;
                return false;
            }
        } else if(command == "flip") {
            if(options.size() < 1) {
                std::cerr << "flip コマンドには vertical, horizontal の指定が必要です" << std::endl;
                return false;
            }
            if(options[0] == "vertical") {
                return manip::flip_vertical(image);
            } else if(options[0] == "horizontal") {
                return manip::flip_horizontal(image);
            } else {
                std::cerr << "flip コマンドには vertical, horizontal の指定が必要です" << std::endl;
                return false;
            }
        } else if(command == "animal") {
            if(options.size() < 2) {
                std::cerr << "animal コマンドにはテキストの指定が2つ必要です" << std::endl;
                return false;
            }
            return animal::animal(image, options[0], options[1]);
        } else if(command == "brain") {
            if(options.size() < 1) {
                std::cerr << "brain コマンドにはテキストの指定が1つ必要です" << std::endl;
                return false;
            }
            return brain::brain(image, ::boost::algorithm::join(options, "\n"));
        }

        std::cerr << "不明なコマンド: " << command << std::endl;
        return false;
    }

    bool parse_and_execute_command(gd &image, const std::string &line) {
        std::vector<std::string> params;

        boost::escaped_list_separator<char> esc("\\", " ", "\"'");
        boost::tokenizer< boost::escaped_list_separator<char> > tokens(line, esc);
        BOOST_FOREACH(const auto &token, tokens) {
            params.push_back(token);
        }
        return execute_command(image, params[0], std::vector<std::string>(params.begin() + 1, params.end()));
    }
}


int main(int argc, char *argv[]) {
    po::variables_map opt;
    if(!handle_program_options(argc, argv, opt)) {
        return 1;
    }

    const std::string in_path(opt["input"].as<std::string>());
    if(in_path == "-") {
        std::cerr << "標準入力を画像入力に使うことはできません" << std::endl;
        return 1;
    }

    std::vector<char> binary;
    if(!load_input_file(in_path, binary)) {
        std::cerr << "指定された入力ファイルから読み込めません" << std::endl;
        return 1;
    }

    if(opt.count("font") == 1) {
        config_waku_font_file = opt["font"].as<std::string>();
    } else {
        config_waku_font_file.clear();
    }
    if(opt.count("font2") == 1) {
        config_animal_font_file1 = opt["font2"].as<std::string>();
    } else {
        config_animal_font_file1.clear();
    }
    if(opt.count("font3") == 1) {
        config_animal_font_file2 = opt["font3"].as<std::string>();
    } else {
        config_animal_font_file2.clear();
    }

    gd current_image;
    try {
        current_image.load_unknown_binary(&binary[0], &binary[0] + binary.size());
    } catch(gd_exception &e) {
        std::cerr << "入力されたファイルは画像でないか対応していません" << std::endl;
        return 1;
    }
    std::vector<char>(0).swap(binary); // free

    std::string line;
    while(!std::cin.eof()) {
        std::getline(std::cin, line);
        if(std::cin.eof()) {
            break;
        }
        boost::trim(line);
        if(!line.empty()) {
            if(!parse_and_execute_command(current_image, line)) {
                return 1;
            }
        }
    }
    
    current_image.save_alpha(true);
    current_image.save_png(binary, 9);
    if(!save_output_file(opt["output"].as<std::string>(), binary)) {
        std::cerr << "指定された出力ファイルへ書き込めません" << std::endl;
        return 1;
    }

    return 0;
}
