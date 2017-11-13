#include "extra/config.h"

#ifdef USE_SFEN_PACKER

#include "kifu_converter.hpp"

#include <experimental/filesystem>
#include <fstream>
#include <iostream>

#include "experimental_learner.h"

using std::experimental::filesystem::directory_iterator;
using std::experimental::filesystem::path;

namespace {
    static constexpr int kBufferSize = 1 << 28;
}

bool KifuConverter::ConvertKifuToText(Position& pos, std::istringstream& ssCmd) {
    std::string binary_folder_path;
    std::string text_folder_path;
    ssCmd >> binary_folder_path >> text_folder_path;

    std::vector<char> input_buffer(kBufferSize);
    std::vector<char> output_buffer(kBufferSize);
    for (directory_iterator next(binary_folder_path), end; next != end; ++next) {
        std::string input_file_path = next->path().generic_string();
        std::ifstream ifs(input_file_path, std::ios::in | std::ios::binary);
        if (!ifs) {
            std::cerr << "Failed to open a binary kifu file. input_file_path=" << input_file_path << std::endl;
            return false;
        }
        ifs.rdbuf()->pubsetbuf(&input_buffer[0], input_buffer.size());
        
        std::string output_file_path = (path(text_folder_path) / (next->path().filename().string() + ".txt")).string();
        std::ofstream ofs(output_file_path);
        if (!ofs) {
            std::cerr << "Failed to open a text kifu file. output_file_path=" << output_file_path << std::endl;
            return false;
        }
        ofs.rdbuf()->pubsetbuf(&output_buffer[0], output_buffer.size());

        std::cerr << input_file_path << " -> " << output_file_path << std::endl;

        int64_t num_records = 0;
        Learner::Record record = { 0 };
        while (ifs.read(reinterpret_cast<char*>(&record), sizeof(record))) {
            if (++num_records % 10000000 == 0) {
                std::cerr << num_records << std::endl;
            }

            pos.set_from_packed_sfen(record.packed);
            ofs << pos.sfen() << std::endl;
            ofs << record.value << std::endl;
            ofs << record.win_color << std::endl;
        }
    }
    return true;
}

bool KifuConverter::ConvertKifuToBinary(Position& pos, std::istringstream& ssCmd) {
    std::string text_folder_path;
    std::string binary_folder_path;
    ssCmd >> text_folder_path >> binary_folder_path;

    std::vector<char> input_buffer(kBufferSize);
    std::vector<char> output_buffer(kBufferSize);
    for (directory_iterator next(text_folder_path), end; next != end; ++next) {
        std::string input_file_path = next->path().generic_string();
        std::ifstream ifs(input_file_path, std::ios::in);
        if (!ifs) {
            std::cerr << "Failed to open a text kifu file. input_file_path=" << input_file_path << std::endl;
            return false;
        }
        ifs.rdbuf()->pubsetbuf(&input_buffer[0], input_buffer.size());

        std::string output_file_path = (path(binary_folder_path) / (next->path().filename().string() + ".bin")).string();
        std::ofstream ofs(output_file_path, std::ios::out | std::ios::binary);
        if (!ofs) {
            std::cerr << "Failed to open a binary kifu file. output_file_path=" << output_file_path << std::endl;
            return false;
        }
        ofs.rdbuf()->pubsetbuf(&output_buffer[0], output_buffer.size());

        std::cerr << input_file_path << " -> " << output_file_path << std::endl;

        std::string sfen;
        while (std::getline(ifs, sfen) && !sfen.empty()) {
            int eval, win;
            ifs >> eval >> win;
            // winÇÃå„ÇÃâ¸çsÇì«Ç›îÚÇŒÇ∑
            std::string _;
            std::getline(ifs, _);

            pos.set(sfen);
            Learner::Record record = { 0 };
            pos.sfen_pack(record.packed);
            record.value = eval;
            record.win_color = win;
            ofs.write(reinterpret_cast<char*>(&record), sizeof(record));
        }
    }
    return true;
}

#endif

