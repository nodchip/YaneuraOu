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
constexpr int kMaxGamePlay = 256;
constexpr int kMaxSwapTrials = 10;
constexpr int kMaxTrialsToSelectSquares = 100;

constexpr char* kOptionGeneratorNumPositions = "GeneratorNumPositions";
constexpr char* kOptionGeneratorMinSearchDepth = "GeneratorMinSearchDepth";
constexpr char* kOptionGeneratorMaxSearchDepth = "GeneratorMaxSearchDepth";
constexpr char* kOptionGeneratorKifuTag = "GeneratorKifuTag";
constexpr char* kOptionGeneratorStartposFileName = "GeneratorStartposFileName";
constexpr char* kOptionGeneratorMinBookMove = "GeneratorMinBookMove";
constexpr char* kOptionGeneratorMaxBookMove = "GeneratorMaxBookMove";
constexpr char* kOptionGeneratorValueThreshold = "GeneratorValueThreshold";
constexpr char* kOptionGeneratorMinMultiPvMoves = "GeneratorMinMultiPvMoves";
constexpr char* kOptionGeneratorMaxMultiPvMoves = "GeneratorMaxMultiPvMoves";
constexpr char* kOptionGeneratorMaxValueDifferenceInMultiPv =
    "GeneratorMaxValueDifferenceInMultiPv";
constexpr char* kOptionConvertSfenToLearningDataInputSfenFileName = "ConvertSfenToLearningDataInputSfenFileName";
constexpr char* kOptionConvertSfenToLearningDataSearchDepth = "ConvertSfenToLearningDataSearchDepth";
constexpr char* kOptionConvertSfenToLearningDataOutputFileName = "ConvertSfenToLearningDataOutputFileName";

std::vector<std::string> book;
std::uniform_real_distribution<> probability_distribution;

bool ReadBook() {
    // 定跡ファイル(というか単なる棋譜ファイル)の読み込み
    std::string book_file_name = Options[kOptionGeneratorStartposFileName];
    std::ifstream fs_book;
    fs_book.open(book_file_name);

    if (!fs_book.is_open()) {
        sync_cout << "Error! : can't read " << book_file_name << sync_endl;
        return false;
    }

    sync_cout << "Reading " << book_file_name << sync_endl;
    std::string line;
    while (!fs_book.eof()) {
        std::getline(fs_book, line);
        if (!line.empty()) book.push_back(line);
        if ((book.size() % 1000) == 0) std::cout << ".";
    }
    std::cout << std::endl;
    sync_cout << "Book size: " << book.size() << sync_endl;
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
    o[kOptionGeneratorMinSearchDepth] << Option(3, 1, MAX_PLY);
    o[kOptionGeneratorMaxSearchDepth] << Option(4, 1, MAX_PLY);
    o[kOptionGeneratorKifuTag] << Option("default_tag");
    o[kOptionGeneratorStartposFileName] << Option("startpos.sfen");
    o[kOptionGeneratorMinBookMove] << Option(0, 1, MAX_PLY);
    o[kOptionGeneratorMaxBookMove] << Option(32, 1, MAX_PLY);
    o[kOptionGeneratorValueThreshold] << Option(32000, 0, VALUE_MATE);
    o[kOptionGeneratorMinMultiPvMoves] << Option(0, 0, MAX_PLY);
    o[kOptionGeneratorMaxMultiPvMoves] << Option(6, 0, MAX_PLY);
    o[kOptionGeneratorMaxValueDifferenceInMultiPv] << Option(100, 0, MAX_PLY);
    o[kOptionConvertSfenToLearningDataInputSfenFileName] << Option("nyugyoku_win.sfen");
    o[kOptionConvertSfenToLearningDataSearchDepth] << Option(12, 1, MAX_PLY);
    o[kOptionConvertSfenToLearningDataOutputFileName] << Option("nyugyoku_win.bin");
}

void Learner::GenerateKifu() {
#ifdef USE_FALSE_PROBE_IN_TT
    static_assert(false, "Please undefine USE_FALSE_PROBE_IN_TT.");
#endif

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 定跡の読み込み
    if (!ReadBook()) {
        sync_cout << "Failed to read the book." << sync_endl;
        return;
    }

    Eval::load_eval();

    omp_set_num_threads((int)Options["Threads"]);

    Search::LimitsType limits;
    // 引き分けの手数付近で引き分けの値が返るのを防ぐため1 << 16にする
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    std::string kifu_directory = (std::string)Options["KifuDir"];
    _mkdir(kifu_directory.c_str());

    int min_search_depth = Options[kOptionGeneratorMaxSearchDepth];
    int max_search_depth = Options[kOptionGeneratorMaxSearchDepth];
    std::uniform_int_distribution<> search_depth_distribution(min_search_depth, max_search_depth);
    int min_book_move = Options[kOptionGeneratorMinBookMove];
    int max_book_move = Options[kOptionGeneratorMaxBookMove];
    std::uniform_int_distribution<> num_book_move_distribution(min_book_move, max_book_move);
    int64_t num_positions = ParseOptionOrDie<int64_t>(kOptionGeneratorNumPositions);
    int value_threshold = Options[kOptionGeneratorValueThreshold];
    std::string output_file_name_tag = Options[kOptionGeneratorKifuTag];
    int min_multi_pv_moves = Options[kOptionGeneratorMinMultiPvMoves];
    int max_multi_pv_moves = Options[kOptionGeneratorMaxMultiPvMoves];
    std::uniform_int_distribution<> multi_pv_moves_distribution(min_multi_pv_moves,
                                                                max_multi_pv_moves);
    int multi_pv = Options["MultiPV"];
    int max_value_difference_in_multi_pv = Options[kOptionGeneratorMaxValueDifferenceInMultiPv];

    std::cout << "min_search_depth=" << min_search_depth << std::endl;
    std::cout << "max_search_depth=" << max_search_depth << std::endl;
    std::cout << "min_book_move=" << min_book_move << std::endl;
    std::cout << "max_book_move=" << max_book_move << std::endl;
    std::cout << "num_positions=" << num_positions << std::endl;
    std::cout << "value_threshold=" << value_threshold << std::endl;
    std::cout << "output_file_name_tag=" << output_file_name_tag << std::endl;
    std::cout << "min_multi_pv_moves=" << min_multi_pv_moves << std::endl;
    std::cout << "max_multi_pv_moves=" << max_multi_pv_moves << std::endl;
    std::cout << "multi_pv=" << multi_pv << std::endl;
    std::cout << "max_value_difference_in_multi_pv=" << max_value_difference_in_multi_pv
              << std::endl;

    time_t start_time;
    std::time(&start_time);
    ASSERT_LV3(book.size());
    std::uniform_int<> opening_index(0, static_cast<int>(book.size() - 1));
    // スレッド間で共有する
    std::atomic_int64_t global_position_index = 0;
    ProgressReport progress_report(num_positions, 10 * 60);
#pragma omp parallel
    {
        int thread_index = ::omp_get_thread_num();
        WinProcGroup::bindThisThread(thread_index);
        char output_file_path[1024];
        std::sprintf(output_file_path, "%s/kifu.%s.%d-%d.%I64d.%03d.bin", kifu_directory.c_str(),
                     output_file_name_tag.c_str(), min_search_depth, max_search_depth,
                     num_positions, thread_index);
        // 各スレッドに持たせる
        std::unique_ptr<Learner::KifuWriter> kifu_writer =
            std::make_unique<Learner::KifuWriter>(output_file_path);
        std::mt19937_64 mt19937_64(start_time + thread_index);

        while (global_position_index < num_positions) {
            Thread& thread = *Threads[thread_index];
            Position& pos = thread.rootPos;
            pos.set_hirate();
            StateInfo state_infos[512] = {0};
            StateInfo* state = state_infos + 8;

            const std::string& opening = book[opening_index(mt19937_64)];
            std::istringstream is(opening);
            std::string token;
            int num_book_move = num_book_move_distribution(mt19937_64);
            while (global_position_index < num_positions && pos.game_ply() < num_book_move) {
                if (!(is >> token)) {
                    break;
                }
                if (token == "startpos" || token == "moves") continue;

                Move m = move_from_usi(pos, token);
                if (!is_ok(m) || !pos.legal(m)) {
                    //  sync_cout << "Error book.sfen , line = " << book_number << " , moves = " <<
                    //  token << endl << rootPos << sync_endl;
                    // →　エラー扱いはしない。
                    break;
                }

                pos.do_move(m, state[pos.game_ply()]);
                // 差分計算のためevaluate()を呼び出す
                Eval::evaluate(pos);
            }

            // pos.game_ply()がこの値未満の場合はmulti pvでsearchし、ランダムに指し手を選択する
            int multi_pv_until_play = pos.game_ply() + multi_pv_moves_distribution(mt19937_64);

            std::vector<Learner::Record> records;
            Value last_value;
            while (pos.game_ply() < kMaxGamePlay && !pos.is_mated() &&
                   pos.DeclarationWin() == MOVE_NONE) {
                pos.set_this_thread(&thread);

                Move pv_move = Move::MOVE_NONE;
                int search_depth = search_depth_distribution(mt19937_64);
                int multi_pv_on_this_play = (pos.game_ply() < multi_pv_until_play ? multi_pv : 1);
                Learner::search(pos, search_depth, multi_pv_on_this_play);
                multi_pv_on_this_play = std::min(
                    multi_pv_on_this_play, static_cast<int>(pos.this_thread()->rootMoves.size()));

                const auto& root_moves = pos.this_thread()->rootMoves;
                int num_valid_root_moves = 0;
                for (int root_move_index = 0; root_move_index < multi_pv_on_this_play;
                     ++root_move_index) {
                    if (root_moves[0].score >
                        root_moves[root_move_index].score + max_value_difference_in_multi_pv) {
                        break;
                    }
                    ++num_valid_root_moves;
                }
                int selected_root_move_index =
                    std::uniform_int_distribution<>(0, num_valid_root_moves - 1)(mt19937_64);
                const auto& root_move = root_moves[selected_root_move_index];
                // 最も良かったスコアをこの局面のスコアとして記録する
                last_value = static_cast<Value>(INT_MIN);
                for (const auto& root_move : root_moves) {
                    last_value = std::max(last_value, root_move.score);
                }
                const std::vector<Move>& pv = root_move.pv;

                // 評価値の絶対値が閾値を超えたら終了する
                if (std::abs(last_value) > value_threshold) {
                    break;
                }

                // 詰みの場合はpvが空になる
                // 上記の条件があるのでこれはいらないかもしれない
                if (pv.empty()) {
                    break;
                }

                // 局面が不正な場合があるので再度チェックする
                if (!pos.pos_is_ok()) {
                    break;
                }

                Learner::Record record = {0};
                pos.sfen_pack(record.packed);
                record.value = last_value;
                records.push_back(record);

                pv_move = pv[0];
                pos.do_move(pv_move, state[pos.game_ply()]);
                // 差分計算のためevaluate()を呼び出す
                Eval::evaluate(pos);
            }

            Color win;
            RepetitionState repetition_state = pos.is_repetition();
            if (pos.is_mated()) {
                // 負け
                // 詰まされた
                win = ~pos.side_to_move();
            } else if (pos.DeclarationWin() != MOVE_NONE) {
                // 勝ち
                // 入玉勝利
                win = pos.side_to_move();
            } else if (last_value > value_threshold) {
                // 勝ち
                win = pos.side_to_move();
            } else if (last_value < -value_threshold) {
                // 負け
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

        // 必要局面数生成したら全スレッドの探索を停止する
        // こうしないと相入玉等合法手の多い局面で止まるまでに時間がかかる
        Search::Signals.stop = true;
    }
}

void Learner::MeasureMoveTimes() {
#ifdef USE_FALSE_PROBE_IN_TT
    static_assert(false, "Please undefine USE_FALSE_PROBE_IN_TT.");
#endif

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 定跡の読み込み
    if (!ReadBook()) {
        sync_cout << "Failed to read the book." << sync_endl;
        return;
    }

    Eval::load_eval();

    Search::LimitsType limits;
    // 引き分けの手数付近で引き分けの値が返るのを防ぐため1 << 16にする
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    std::string kifu_directory = (std::string)Options["KifuDir"];
    _mkdir(kifu_directory.c_str());

    int min_search_depth = Options[kOptionGeneratorMaxSearchDepth];
    int max_search_depth = Options[kOptionGeneratorMaxSearchDepth];
    std::uniform_int_distribution<> search_depth_distribution(min_search_depth, max_search_depth);
    int min_book_move = Options[kOptionGeneratorMinBookMove];
    int max_book_move = Options[kOptionGeneratorMaxBookMove];
    std::uniform_int_distribution<> num_book_move_distribution(min_book_move, max_book_move);
    int64_t num_positions = ParseOptionOrDie<int64_t>(kOptionGeneratorNumPositions);
    int value_threshold = Options[kOptionGeneratorValueThreshold];
    std::string output_file_name_tag = Options[kOptionGeneratorKifuTag];
    int min_multi_pv_moves = Options[kOptionGeneratorMinMultiPvMoves];
    int max_multi_pv_moves = Options[kOptionGeneratorMaxMultiPvMoves];
    std::uniform_int_distribution<> multi_pv_moves_distribution(min_multi_pv_moves,
                                                                max_multi_pv_moves);
    int multi_pv = Options["MultiPV"];
    int max_value_difference_in_multi_pv = Options[kOptionGeneratorMaxValueDifferenceInMultiPv];

    std::cout << "min_search_depth=" << min_search_depth << std::endl;
    std::cout << "max_search_depth=" << max_search_depth << std::endl;
    std::cout << "min_book_move=" << min_book_move << std::endl;
    std::cout << "max_book_move=" << max_book_move << std::endl;
    std::cout << "num_positions=" << num_positions << std::endl;
    std::cout << "value_threshold=" << value_threshold << std::endl;
    std::cout << "output_file_name_tag=" << output_file_name_tag << std::endl;
    std::cout << "min_multi_pv_moves=" << min_multi_pv_moves << std::endl;
    std::cout << "max_multi_pv_moves=" << max_multi_pv_moves << std::endl;
    std::cout << "multi_pv=" << multi_pv << std::endl;
    std::cout << "max_value_difference_in_multi_pv=" << max_value_difference_in_multi_pv
              << std::endl;

    time_t start_time;
    std::time(&start_time);
    ASSERT_LV3(book.size());
    std::uniform_int<> opening_index(0, static_cast<int>(book.size() - 1));
    // スレッド間で共有する
    std::atomic_int64_t global_position_index = 0;

    int thread_index = ::omp_get_thread_num();
    WinProcGroup::bindThisThread(thread_index);
    char output_file_path[1024];
    std::sprintf(output_file_path, "%s/kifu.%s.%d-%d.%I64d.%03d.bin", kifu_directory.c_str(),
                 output_file_name_tag.c_str(), min_search_depth, max_search_depth, num_positions,
                 thread_index);
    // 各スレッドに持たせる
    std::mt19937_64 mt19937_64(start_time + thread_index);

    std::ofstream ofs("measure_move_times.log.12.txt");

    while (global_position_index < num_positions) {
        Thread& thread = *Threads[thread_index];
        Position& pos = thread.rootPos;
        pos.set_hirate();
        StateInfo state_infos[512] = {0};
        StateInfo* state = state_infos + 8;

        const std::string& opening = book[opening_index(mt19937_64)];
        std::istringstream is(opening);
        std::string token;
        int num_book_move = num_book_move_distribution(mt19937_64);
        while (global_position_index < num_positions && pos.game_ply() < num_book_move) {
            if (!(is >> token)) {
                break;
            }
            if (token == "startpos" || token == "moves") continue;

            Move m = move_from_usi(pos, token);
            if (!is_ok(m) || !pos.legal(m)) {
                //  sync_cout << "Error book.sfen , line = " << book_number << " , moves = " <<
                //  token << endl << rootPos << sync_endl;
                // →　エラー扱いはしない。
                break;
            }

            pos.do_move(m, state[pos.game_ply()]);
            // 差分計算のためevaluate()を呼び出す
            Eval::evaluate(pos);
        }

        // pos.game_ply()がこの値未満の場合はmulti pvでsearchし、ランダムに指し手を選択する
        int multi_pv_until_play = pos.game_ply() + multi_pv_moves_distribution(mt19937_64);

        struct PositionRecord {
            int play;
            int time_ms;
        };

        std::vector<PositionRecord> position_records;

        Value last_value;
        while (pos.game_ply() < kMaxGamePlay && !pos.is_mated() &&
               pos.DeclarationWin() == MOVE_NONE) {
            pos.set_this_thread(&thread);

            Move pv_move = Move::MOVE_NONE;
            int search_depth = search_depth_distribution(mt19937_64);
            int multi_pv_on_this_play = (pos.game_ply() < multi_pv_until_play ? multi_pv : 1);

            auto start_time = std::chrono::system_clock::now();
            Learner::search(pos, search_depth, multi_pv_on_this_play);
            auto end_time = std::chrono::system_clock::now();

            if (multi_pv_on_this_play == 1) {
                position_records.push_back(
                    {pos.game_ply(),
                     static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          end_time - start_time)
                                          .count())});
            }

            multi_pv_on_this_play = std::min(multi_pv_on_this_play,
                                             static_cast<int>(pos.this_thread()->rootMoves.size()));

            const auto& root_moves = pos.this_thread()->rootMoves;
            int num_valid_root_moves = 0;
            for (int root_move_index = 0; root_move_index < multi_pv_on_this_play;
                 ++root_move_index) {
                if (root_moves[0].score >
                    root_moves[root_move_index].score + max_value_difference_in_multi_pv) {
                    break;
                }
                ++num_valid_root_moves;
            }
            int selected_root_move_index =
                std::uniform_int_distribution<>(0, num_valid_root_moves - 1)(mt19937_64);
            const auto& root_move = root_moves[selected_root_move_index];
            last_value = root_move.score;
            const std::vector<Move>& pv = root_move.pv;

            // 評価値の絶対値が閾値を超えたら終了する
            if (std::abs(last_value) > value_threshold) {
                break;
            }

            // 詰みの場合はpvが空になる
            // 上記の条件があるのでこれはいらないかもしれない
            if (pv.empty()) {
                break;
            }

            // 局面が不正な場合があるので再度チェックする
            if (!pos.pos_is_ok()) {
                break;
            }

            pv_move = pv[0];
            pos.do_move(pv_move, state[pos.game_ply()]);
            // 差分計算のためevaluate()を呼び出す
            Eval::evaluate(pos);
        }

        Color win;
        RepetitionState repetition_state = pos.is_repetition();
        if (pos.is_mated()) {
            // 負け
            // 詰まされた
            win = ~pos.side_to_move();
        } else if (pos.DeclarationWin() != MOVE_NONE) {
            // 勝ち
            // 入玉勝利
            win = pos.side_to_move();
        } else if (last_value > value_threshold) {
            // 勝ち
            win = pos.side_to_move();
        } else if (last_value < -value_threshold) {
            // 負け
            win = ~pos.side_to_move();
        } else {
            continue;
        }

        for (const auto& position_record : position_records) {
            ofs << position_record.play << "," << position_record.time_ms << std::endl;
            std::cout << position_record.play << "," << position_record.time_ms << std::endl;
        }
        global_position_index += position_records.size();
        std::cout << "!!!!! " << global_position_index << std::endl;
    }

    // 必要局面数生成したら全スレッドの探索を停止する
    // こうしないと相入玉等合法手の多い局面で止まるまでに時間がかかる
    Search::Signals.stop = true;
}

void Learner::ConvertSfenToLearningData() {
#ifdef USE_FALSE_PROBE_IN_TT
    static_assert(false, "Please undefine USE_FALSE_PROBE_IN_TT.");
#endif

    Eval::load_eval();

    omp_set_num_threads((int)Options["Threads"]);

    Search::LimitsType limits;
    // 引き分けの手数付近で引き分けの値が返るのを防ぐため1 << 16にする
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

    // スレッド間で共有する
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
            StateInfo state_infos[2048] = { 0 };
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
