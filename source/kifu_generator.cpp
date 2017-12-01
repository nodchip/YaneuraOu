#ifdef USE_KIFU_GENERATOR

#include "kifu_generator.h"

#include <direct.h>
#include <omp.h>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>

#include "kifu_writer.h"
#include "misc.h"
#include "progress_report.h"
#include "search.h"
#include "thread.h"

#ifdef abs
#undef abs
#endif

using Search::RootMove;
using USI::Option;
using USI::OptionsMap;

namespace Learner {
std::pair<Value, std::vector<Move> > search(Position& pos, int depth, size_t multiPV = 1);
std::pair<Value, std::vector<Move> > qsearch(Position& pos);
}

namespace {
constexpr int kMaxGamePlay = 400;
constexpr int kMaxSwapTrials = 10;
constexpr int kMaxTrialsToSelectSquares = 100;

constexpr char* kOptionGeneratorNumPositions = "GeneratorNumPositions";
constexpr char* kOptionGeneratorSearchDepth = "GeneratorSearchDepth";
constexpr char* kOptionGeneratorKifuTag = "GeneratorKifuTag";
constexpr char* kOptionGeneratorStartposFileName = "GeneratorStartposFileName";
constexpr char* kOptionGeneratorValueThreshold = "GeneratorValueThreshold";
constexpr char* kOptionConvertSfenToLearningDataInputSfenFileName = "ConvertSfenToLearningDataInputSfenFileName";
constexpr char* kOptionConvertSfenToLearningDataSearchDepth = "ConvertSfenToLearningDataSearchDepth";
constexpr char* kOptionConvertSfenToLearningDataOutputFileName = "ConvertSfenToLearningDataOutputFileName";

std::vector<std::string> start_positions;
std::uniform_real_distribution<> probability_distribution;

bool ReadBook() {
    // ��Ճt�@�C��(�Ƃ������P�Ȃ�����t�@�C��)�̓ǂݍ���
    std::string book_file_name = Options[kOptionGeneratorStartposFileName];
    std::ifstream fs_book;
    fs_book.open(book_file_name);

    if (!fs_book.is_open()) {
        sync_cout << "Error! : can't read " << book_file_name << sync_endl;
        return false;
    }

    sync_cout << "Reading " << book_file_name << sync_endl;
    std::string line;
    int line_index = 0;
    while (!fs_book.eof()) {
        Thread& thread = *Threads[0];
        Position& pos = thread.rootPos;
        pos.set_hirate();
        StateInfo state_infos[4096] = { 0 };
        StateInfo* state = state_infos + 8;

        std::getline(fs_book, line);
        std::istringstream is(line);
        std::string token;
        for (;;) {
            if (!(is >> token)) {
                break;
            }
            if (token == "startpos" || token == "moves") continue;

            Move m = move_from_usi(pos, token);
            if (!is_ok(m) || !pos.legal(m)) {
                //  sync_cout << "Error book.sfen , line = " << book_number << " , moves = " <<
                //  token << endl << rootPos << sync_endl;
                // ���@�G���[�����͂��Ȃ��B
                break;
            }

            pos.do_move(m, state[pos.game_ply()]);
            start_positions.push_back(pos.sfen());
        }

        if ((++line_index % 1000) == 0) std::cout << ".";
    }
    std::cout << std::endl;
    sync_cout << "Number of lines: " << line_index << sync_endl;
    sync_cout << "Number of start positions: " << start_positions.size() << sync_endl;
    return true;
}

template <typename T>
T ParseOptionOrDie(const char* name) {
    std::string value_string = (std::string)Options[name];
    std::istringstream iss(value_string);
    T value;
    if (!(iss >> value)) {
        sync_cout << "Failed to parse an option. Exitting...: name=" << name << " value=" << value
                  << sync_endl;
        std::exit(1);
    }
    return value;
}
}

void Learner::InitializeGenerator(USI::OptionsMap& o) {
    o[kOptionGeneratorNumPositions] << Option("10000000000");
    o[kOptionGeneratorSearchDepth] << Option(8, 1, MAX_PLY);
    o[kOptionGeneratorKifuTag] << Option("default_tag");
    o[kOptionGeneratorStartposFileName] << Option("startpos.sfen");
    o[kOptionGeneratorValueThreshold] << Option(VALUE_MATE, 0, VALUE_MATE);
    o[kOptionConvertSfenToLearningDataInputSfenFileName] << Option("nyugyoku_win.sfen");
    o[kOptionConvertSfenToLearningDataSearchDepth] << Option(12, 1, MAX_PLY);
    o[kOptionConvertSfenToLearningDataOutputFileName] << Option("nyugyoku_win.bin");
}

namespace {
    void RandomMove(Position& pos, StateInfo* state, std::mt19937_64& mt) {
        ASSERT_LV3(pos.this_thread());
        auto& root_moves = pos.this_thread()->rootMoves;

        root_moves.clear();
        for (auto m : MoveList<LEGAL>(pos)) {
            root_moves.push_back(Search::RootMove(m));
        }

        if (root_moves.empty()) {
            return;
        }

        std::uniform_int_distribution<> dist(0, static_cast<int>(root_moves.size()) - 1);
        pos.do_move(root_moves[dist(mt)].pv[0], state[pos.game_ply()]);
        Eval::evaluate(pos);
    }
}

void Learner::GenerateKifu() {
#ifdef USE_FALSE_PROBE_IN_TT
    static_assert(false, "Please undefine USE_FALSE_PROBE_IN_TT.");
#endif

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // ��Ղ̓ǂݍ���
    if (!ReadBook()) {
        sync_cout << "Failed to read the book." << sync_endl;
        return;
    }

    Eval::load_eval();

    omp_set_num_threads((int)Options["Threads"]);

    Search::LimitsType limits;
    // ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    std::string kifu_directory = (std::string)Options["KifuDir"];
    _mkdir(kifu_directory.c_str());

    int search_depth = Options[kOptionGeneratorSearchDepth];
    int64_t num_positions = ParseOptionOrDie<int64_t>(kOptionGeneratorNumPositions);
    int value_threshold = Options[kOptionGeneratorValueThreshold];
    std::string output_file_name_tag = Options[kOptionGeneratorKifuTag];

    std::cout << "search_depth=" << search_depth << std::endl;
    std::cout << "num_positions=" << num_positions << std::endl;
    std::cout << "value_threshold=" << value_threshold << std::endl;
    std::cout << "output_file_name_tag=" << output_file_name_tag << std::endl;

    time_t start_time;
    std::time(&start_time);
    ASSERT_LV3(start_positions.size());
    std::uniform_int<> start_positions_index(0, static_cast<int>(start_positions.size() - 1));
    // �X���b�h�Ԃŋ��L����
    std::atomic_int64_t global_position_index = 0;
    ProgressReport progress_report(num_positions, 10 * 60);
#pragma omp parallel
    {
        int thread_index = ::omp_get_thread_num();
        WinProcGroup::bindThisThread(thread_index);
        char output_file_path[1024];
        std::sprintf(output_file_path, "%s/kifu.%s.%d.%I64d.%03d.%I64d.bin", kifu_directory.c_str(),
                     output_file_name_tag.c_str(), search_depth,
                     num_positions, thread_index, start_time);
        // �e�X���b�h�Ɏ�������
        std::unique_ptr<Learner::KifuWriter> kifu_writer =
            std::make_unique<Learner::KifuWriter>(output_file_path);
        std::mt19937_64 mt19937_64(start_time + thread_index);

        while (global_position_index < num_positions) {
            Thread& thread = *Threads[thread_index];
            Position& pos = thread.rootPos;
            pos.set(start_positions[start_positions_index(mt19937_64)]);
            pos.set_this_thread(&thread);
            StateInfo state_infos[4096] = {0};
            StateInfo* state = state_infos + 8;

            RandomMove(pos, state, mt19937_64);

            std::vector<Learner::Record> records;
            Value last_value;
            while (pos.game_ply() < kMaxGamePlay && !pos.is_mated() &&
                   pos.DeclarationWin() == MOVE_NONE) {
                pos.set_this_thread(&thread);

                Move pv_move = Move::MOVE_NONE;
                Learner::search(pos, search_depth);

                const auto& root_moves = pos.this_thread()->rootMoves;
                const auto& root_move = root_moves[0];
                // �ł��ǂ������X�R�A�����̋ǖʂ̃X�R�A�Ƃ��ċL�^����
                last_value = root_move.score;
                const std::vector<Move>& pv = root_move.pv;

                // �]���l�̐�Βl��臒l�𒴂�����I������
                if (std::abs(last_value) > value_threshold) {
                    break;
                }

                // �l�݂̏ꍇ��pv����ɂȂ�
                // ��L�̏���������̂ł���͂���Ȃ���������Ȃ�
                if (pv.empty()) {
                    break;
                }

                // �ǖʂ��s���ȏꍇ������̂ōēx�`�F�b�N����
                if (!pos.pos_is_ok()) {
                    break;
                }

                Learner::Record record = {0};
                pos.sfen_pack(record.packed);
                record.value = last_value;
                records.push_back(record);

                pv_move = pv[0];
                pos.do_move(pv_move, state[pos.game_ply()]);
                // �����v�Z�̂���evaluate()���Ăяo��
                Eval::evaluate(pos);
            }

            Color win;
            RepetitionState repetition_state = pos.is_repetition();
            if (pos.is_mated()) {
                // ����
                // �l�܂��ꂽ
                win = ~pos.side_to_move();
            } else if (pos.DeclarationWin() != MOVE_NONE) {
                // ����
                // ���ʏ���
                win = pos.side_to_move();
            } else if (last_value > value_threshold) {
                // ����
                win = pos.side_to_move();
            } else if (last_value < -value_threshold) {
                // ����
                win = ~pos.side_to_move();
            } else {
                continue;
            }

            for (auto& record : records) {
                record.win_color = win;
            }

            for (const auto& record : records) {
                if (!kifu_writer->Write(record)) {
                    sync_cout << "info string Failed to write a record." << sync_endl;
                    std::exit(1);
                }
            }

            progress_report.Show(global_position_index += records.size());
        }

        // �K�v�ǖʐ�����������S�X���b�h�̒T�����~����
        // �������Ȃ��Ƒ����ʓ����@��̑����ǖʂŎ~�܂�܂łɎ��Ԃ�������
        Search::Signals.stop = true;
    }
}

void Learner::ConvertSfenToLearningData() {
#ifdef USE_FALSE_PROBE_IN_TT
    static_assert(false, "Please undefine USE_FALSE_PROBE_IN_TT.");
#endif

    Eval::load_eval();

    omp_set_num_threads((int)Options["Threads"]);

    Search::LimitsType limits;
    // ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    std::string input_sfen_file_name = (std::string)Options[kOptionConvertSfenToLearningDataInputSfenFileName];
    int search_depth = Options[kOptionConvertSfenToLearningDataSearchDepth];
    std::string output_file_name = Options[kOptionConvertSfenToLearningDataOutputFileName];

    std::cout << "input_sfen_file_name=" << input_sfen_file_name << std::endl;
    std::cout << "search_depth=" << search_depth << std::endl;
    std::cout << "output_file_name=" << output_file_name << std::endl;

    time_t start_time;
    std::time(&start_time);

    std::vector<std::string> sfens;
    {
        std::ifstream ifs(input_sfen_file_name);
        std::string sfen;
        while (std::getline(ifs, sfen)) {
            sfens.push_back(sfen);
        }
    }

    // �X���b�h�Ԃŋ��L����
    std::atomic_int64_t global_sfen_index = 0;
    int64_t num_sfens = sfens.size();
    ProgressReport progress_report(num_sfens, 60);
    std::unique_ptr<Learner::KifuWriter> kifu_writer =
        std::make_unique<Learner::KifuWriter>(output_file_name);
    std::mutex mutex;
#pragma omp parallel
    {
        int thread_index = ::omp_get_thread_num();
        WinProcGroup::bindThisThread(thread_index);

        for (int64_t sfen_index = global_sfen_index++; sfen_index < num_sfens; sfen_index = global_sfen_index++) {
            const std::string& sfen = sfens[sfen_index];
            Thread& thread = *Threads[thread_index];
            Position& pos = thread.rootPos;
            pos.set_hirate();
            pos.set_this_thread(&thread);
            StateInfo state_infos[4096] = { 0 };
            StateInfo* state = state_infos + 8;

            std::istringstream iss(sfen);
            // startpos moves 7g7f 3c3d 2g2f
            std::vector<Learner::Record> records;
            std::string token;
            Color win = COLOR_NB;
            while (iss >> token) {
                if (token == "startpos" || token == "moves") {
                    continue;
                }

                Move m = move_from_usi(pos, token);
                if (!is_ok(m) || !pos.legal(m)) {
                    break;
                }

                pos.do_move(m, state[pos.game_ply()]);

                Learner::search(pos, search_depth);
                const auto& root_moves = pos.this_thread()->rootMoves;
                const auto& root_move = root_moves[0];

                Learner::Record record = { 0 };
                pos.sfen_pack(record.packed);
                record.value = root_move.score;
                records.push_back(record);

                if (pos.DeclarationWin()) {
                    win = pos.side_to_move();
                    break;
                }
            }

            //sync_cout << pos << sync_endl;
            //pos.DeclarationWin();

            if (win == COLOR_NB) {
                sync_cout << "Skipped..." << sync_endl;
                continue;
            }
            sync_cout << "DeclarationWin..." << sync_endl;

            for (auto& record : records) {
                record.win_color = win;
            }

            std::lock_guard<std::mutex> lock_gurad(mutex);
            {
                for (const auto& record : records) {
                    if (!kifu_writer->Write(record)) {
                        sync_cout << "info string Failed to write a record." << sync_endl;
                        std::exit(1);
                    }
                }
            }

            progress_report.Show(global_sfen_index);
        }
    }
}

#endif  // USE_KIFU_GENERATOR
