#include "kifu_converter.hpp"

#include <experimental/filesystem>
#include <fstream>
#include <iostream>

#include "position.hpp"
#include "search.hpp"
#include "thread.hpp"

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
        HuffmanCodedPosAndEval record = { 0 };
        while (!ifs.eof()) {
            if (++num_records % 10000000 == 0) {
                std::cerr << num_records << std::endl;
            }

            ifs.read(reinterpret_cast<char*>(&record), sizeof(record));
            if (record.gameResult != BlackWin && record.gameResult != WhiteWin) {
                // 引き分け等をスキップする
                continue;
            }

            pos.set(record.hcp, pos.searcher()->threads.main());
            ofs << pos.toSFEN() << std::endl;
            ofs << record.eval << std::endl;
            switch (record.gameResult) {
            case BlackWin:
                ofs << 0 << std::endl;
                break;
            case WhiteWin:
                ofs << 1 << std::endl;
                break;
            default:
                std::cerr << "Unknown game result. record.gameResult=" << record.gameResult << std::endl;
                return false;
            }
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
            // winの後の改行を読み飛ばす
            std::string _;
            std::getline(ifs, _);

            pos.set(sfen.substr(5), pos.searcher()->threads.main());
            HuffmanCodedPosAndEval record = { 0 };
            record.hcp = pos.toHuffmanCodedPos();
            record.eval = static_cast<s16>(eval);
            switch (win) {
            case 0:
                record.gameResult = BlackWin;
                break;
            case 1:
                record.gameResult = WhiteWin;
                break;
            default:
                std::cerr << "Unknown game result. record.gameResult=" << record.gameResult << std::endl;
                return false;
            }

            ofs.write(reinterpret_cast<char*>(&record), sizeof(record));
        }
    }
    return true;
}
