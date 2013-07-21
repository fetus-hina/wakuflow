#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <errno.h>
#include "gd.h"
#include "waku.h"
#include "waku_v2.h"
#include "manip.h"

namespace po = ::boost::program_options;

namespace {
    bool handle_program_options(int argc, char *argv[], po::variables_map &vm) {
        po::options_description opt("Options");
        opt.add_options()
            ("input,i",  po::value<std::string>(), "Input file path")
            ("output,o", po::value<std::string>(), "Output file path")
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
            //TODO: screen_name
            return waku_v2::waku_v2(image);
        } else if(command == "grayscale") {
            return manip::grayscale(image);
        }
        std::cerr << "不明なコマンド: " << command << std::endl;
        return false;
    }

    bool parse_and_execute_command(gd &image, const std::string &line) {
        // 真面目にトークン分割考えたほうがよさげ
        std::vector<std::string> tokens;
        boost::split(tokens, line, boost::is_space());
        if(tokens.size() < 1) {
            return true;
        }
        return execute_command(image, tokens[0], std::vector<std::string>(tokens.begin() + 1, tokens.end()));
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
    
    current_image.save_png(binary, 9);
    if(!save_output_file(opt["output"].as<std::string>(), binary)) {
        std::cerr << "指定された出力ファイルへ書き込めません" << std::endl;
        return 1;
    }

    return 0;
}
