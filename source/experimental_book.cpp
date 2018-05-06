#include "experimental_book.h"

#include <atomic>
#include <fstream>
#include <sstream>
#include <set>

#include "extra/book/book.h"
#include "misc.h"
#include "position.h"
#include "progress_report.h"
#include "thread.h"

#ifdef EVAL_LEARN

using USI::Option;

namespace {
    constexpr char* kBookSfenFile = "BookSfenFile";
    constexpr char* kBookMaxMoves = "BookMaxMoves";
    constexpr char* kBookFile = "BookFile";
    constexpr char* kBookSearchDepth = "BookSearchDepth";
    constexpr char* kBookInputFile = "BookInputFile";
    constexpr char* kBookOutputFile = "BookOutputFile";
    constexpr char* kBookSavePerPositions = "BookSavePerPositions";
    constexpr char* kThreads = "Threads";
    constexpr char* kMultiPV = "MultiPV";
    constexpr char* kBookOverwriteExistingPositions = "OverwriteExistingPositions";
    constexpr char* kBookNarrowBook = "NarrowBook";
    constexpr char* kBookSkipIfScored = "BookSkipIfScored";
    constexpr int kShowProgressAtMostSec = 10 * 60; // 10 minutes
    constexpr int kSaveEvalAtMostSec = 1 * 60; // 1 minutes
}

namespace Learner {
    std::pair<Value, std::vector<Move> > search(Position& pos, int depth, size_t multiPV = 1);
}

bool Book::Initialize(USI::OptionsMap& o) {
    o[kBookSfenFile] << Option("merged.sfen");
    o[kBookMaxMoves] << Option(32, 0, 256);
    o[kBookSearchDepth] << Option(24, 0, 256);
    o[kBookInputFile] << Option("user_book1.db");
    o[kBookOutputFile] << Option("user_book2.db");
    o[kBookSavePerPositions] << Option(1000, 1, std::numeric_limits<int>::max());
    o[kBookOverwriteExistingPositions] << Option(false);
    o[kBookSkipIfScored] << Option(false);
    return true;
}

bool Book::CreateRawBook() {
    Search::LimitsType limits;
    // 引き分けの手数付近で引き分けの値が返るのを防ぐため1 << 16にする
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    std::string sfen_file = Options[kBookSfenFile];
    int max_moves = (int)Options[kBookMaxMoves];
    bool narrow_book = static_cast<bool>(Options[kBookNarrowBook]);

    std::ifstream ifs(sfen_file);
    if (!ifs) {
        sync_cout << "info string Failed to open the sfen file." << sync_endl;
        std::exit(-1);
    }
    std::string line;

    StateInfo state_info[4096] = { 0 };
    StateInfo* state = state_info + 8;
    std::map<std::string, int> sfen_to_count;
    int num_records = 0;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        Position pos;
        pos.set_hirate(state, Threads[0]);
        ++sfen_to_count[pos.sfen()];

        std::string token;
        int num_moves = 0;
        while (num_moves < max_moves && iss >> token) {
            if (token == "startpos" || token == "moves") {
                continue;
            }
            Move move = move_from_usi(pos, token);
            pos.do_move(move, state[num_moves]);
            ++sfen_to_count[pos.sfen()];
            ++num_moves;
        }

        ++num_records;
        if (num_records % 10000 == 0) {
            sync_cout << "info string " << num_records << sync_endl;
        }
    }

    sync_cout << "info string |sfen_to_count|=" << sfen_to_count.size() << sync_endl;

    std::vector<std::pair<std::string, int> > narrowed_book(sfen_to_count.begin(),
        sfen_to_count.end());
    if (narrow_book) {
        narrowed_book.erase(std::remove_if(narrowed_book.begin(), narrowed_book.end(),
            [](const auto& x) { return x.second == 1; }),
            narrowed_book.end());
    }
    sync_cout << "info string |sfen_to_count|=" << narrowed_book.size() << " (narrow)" << sync_endl;

    MemoryBook memory_book;
    for (const auto& sfen_and_count : narrowed_book) {
        const std::string& sfen = sfen_and_count.first;
        int count = sfen_and_count.second;
        BookPos book_pos(Move::MOVE_NONE, Move::MOVE_NONE, 0, 0, count);
        memory_book.insert(sfen, book_pos);
    }

    std::string book_file = Options[kBookFile];
    book_file = "book/" + book_file;
    memory_book.write_book(book_file, true);

    return true;
}

bool Book::CreateScoredBook() {
    Search::LimitsType limits;
    // 引き分けの手数付近で引き分けの値が返るのを防ぐため1 << 16にする
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    int num_threads = (int)Options[kThreads];
    std::string input_book_file = Options[kBookInputFile];
    int search_depth = (int)Options[kBookSearchDepth];
    int multi_pv = (int)Options[kMultiPV];
    std::string output_book_file = Options[kBookOutputFile];
    int save_per_positions = (int)Options[kBookSavePerPositions];
    bool overwrite_existing_positions = static_cast<bool>(Options[kBookOverwriteExistingPositions]);

    sync_cout << "info string num_threads=" << num_threads << sync_endl;
    sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
    sync_cout << "info string search_depth=" << search_depth << sync_endl;
    sync_cout << "info string multi_pv=" << multi_pv << sync_endl;
    sync_cout << "info string output_book_file=" << output_book_file << sync_endl;
    sync_cout << "info string save_per_positions=" << save_per_positions << sync_endl;
    sync_cout << "info string overwrite_existing_positions=" << overwrite_existing_positions
        << sync_endl;

    MemoryBook input_book;
    input_book_file = "book/" + input_book_file;
    sync_cout << "Reading input book file: " << input_book_file << sync_endl;
    input_book.read_book(input_book_file);
    sync_cout << "done..." << sync_endl;
    sync_cout << "|input_book|=" << input_book.book_body.size() << sync_endl;

    MemoryBook output_book;
    output_book_file = "book/" + output_book_file;
    sync_cout << "Reading output book file: " << output_book_file << sync_endl;
    output_book.read_book(output_book_file);
    sync_cout << "done..." << sync_endl;
    sync_cout << "|output_book|=" << output_book.book_body.size() << sync_endl;

    std::vector<std::string> sfens;
    for (const auto& sfen_and_count : input_book.book_body) {
        if (!overwrite_existing_positions &&
            output_book.book_body.find(sfen_and_count.first) != output_book.book_body.end()) {
            continue;
        }
        sfens.push_back(sfen_and_count.first);
    }
    int num_sfens = static_cast<int>(sfens.size());
    sync_cout << "Number of the positions to be processed: " << num_sfens << sync_endl;

    time_t start_time = 0;
    std::time(&start_time);
    time_t last_save_time = start_time;

    std::atomic_int global_position_index(0);
    std::mutex output_book_mutex;
    ProgressReport progress_report(num_sfens, kShowProgressAtMostSec);

    std::vector<std::thread> threads;
    std::atomic_int global_pos_index(0);
    std::atomic_int global_num_processed_positions(0);
    for (int thread_index = 0; thread_index < num_threads; ++thread_index) {
        threads.push_back(std::thread([thread_index, num_sfens, search_depth, multi_pv,
            save_per_positions, &global_pos_index, &sfens, &output_book,
            &output_book_mutex, &progress_report, &output_book_file,
            &global_num_processed_positions, &last_save_time]() {
            WinProcGroup::bindThisThread(thread_index);

            for (int position_index = global_pos_index++; position_index < num_sfens;
                position_index = global_pos_index++) {
                const std::string& sfen = sfens[position_index];
                Thread& thread = *Threads[thread_index];
                StateInfo state_info = { 0 };
                Position& pos = thread.rootPos;
                pos.set(sfen, &state_info, &thread);

                if (pos.is_mated()) {
                    continue;
                }

                Learner::search(pos, search_depth, multi_pv);

                int num_pv = std::min(multi_pv, static_cast<int>(thread.rootMoves.size()));
                for (int pv_index = 0; pv_index < num_pv; ++pv_index) {
                    const auto& root_move = thread.rootMoves[pv_index];
                    Move best = Move::MOVE_NONE;
                    if (root_move.pv.size() >= 1) {
                        best = root_move.pv[0];
                    }
                    Move next = Move::MOVE_NONE;
                    if (root_move.pv.size() >= 2) {
                        next = root_move.pv[1];
                    }
                    int value = root_move.score;
                    BookPos book_pos(best, next, value, search_depth, 0);
                    {
                        std::lock_guard<std::mutex> lock(output_book_mutex);
                        output_book.insert(sfen, book_pos);
                    }
                }

                int num_processed_positions = ++global_num_processed_positions;
                progress_report.Show(num_processed_positions);

                time_t now = std::time(nullptr);
                if (num_processed_positions && num_processed_positions % save_per_positions == 0 ||
                    now - last_save_time > kSaveEvalAtMostSec) {
                    std::lock_guard<std::mutex> lock(output_book_mutex);
                    if (num_processed_positions && num_processed_positions % save_per_positions == 0 ||
                        now - last_save_time > kSaveEvalAtMostSec) {
                        sync_cout << "Writing the book file..." << sync_endl;
                        output_book.write_book(output_book_file, false);
                        last_save_time = now;
                        sync_cout << "done..." << sync_endl;
                    }
                }
            }
        }));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    sync_cout << "Writing the book file..." << sync_endl;
    output_book.write_book(output_book_file, false);
    sync_cout << "done..." << sync_endl;

    return true;
}

bool Book::AddScoreToBook() {
    Search::LimitsType limits;
    // 引き分けの手数付近で引き分けの値が返るのを防ぐため1 << 16にする
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    int num_threads = (int)Options[kThreads];
    std::string book_file = Options[kBookFile];
    int search_depth = (int)Options[kBookSearchDepth];
    int save_per_positions = (int)Options[kBookSavePerPositions];
    bool skip_if_scored = (bool)Options[kBookSkipIfScored];

    sync_cout << "info string num_threads=" << num_threads << sync_endl;
    sync_cout << "info string book_file=" << book_file << sync_endl;
    sync_cout << "info string search_depth=" << search_depth << sync_endl;
    sync_cout << "info string save_per_positions=" << save_per_positions << sync_endl;
    sync_cout << "info string skip_if_scored=" << skip_if_scored << sync_endl;

    MemoryBook book;
    book_file = "book/" + book_file;
    sync_cout << "Reading input book file: " << book_file << sync_endl;
    book.read_book(book_file);
    sync_cout << "done..." << sync_endl;
    sync_cout << "|input_book|=" << book.book_body.size() << sync_endl;

    // secondがshared_ptrになっている点に気をつける
    std::vector<std::pair<std::string, PosMoveListPtr>> book_positions(book.book_body.begin(), book.book_body.end());

    time_t start_time = 0;
    std::time(&start_time);
    time_t last_save_time = start_time;

    std::atomic_int global_position_index(0);
    std::mutex output_book_mutex;
    ProgressReport progress_report(book_positions.size(), kShowProgressAtMostSec);

    std::vector<std::thread> threads;
    std::atomic_int global_pos_index(0);
    std::atomic_int global_num_processed_positions(0);
    for (int thread_index = 0; thread_index < num_threads; ++thread_index) {
        threads.push_back(std::thread([thread_index, search_depth,
            save_per_positions, skip_if_scored, &global_pos_index,
            &output_book_mutex, &progress_report, &book_file,
            &global_num_processed_positions, &book_positions, &book, &last_save_time]() {
            WinProcGroup::bindThisThread(thread_index);

            for (int position_index = global_pos_index++; position_index < static_cast<int>(book_positions.size());
                position_index = global_pos_index++) {
                const std::string& sfen = book_positions[position_index].first;
                Thread& thread = *Threads[thread_index];
                StateInfo state_info[2] = { 0 };
                Position& pos = thread.rootPos;
                pos.set(sfen, &state_info[0], &thread);

                if (pos.is_mated()) {
                    book.book_body.erase(sfen);
                    continue;
                }

                bool scored = false;
                for (auto& book_move : *book_positions[position_index].second) {
                    //sync_cout << "book_move.value=" << book_move.value << sync_endl;
                    if (skip_if_scored && book_move.value != 0) {
                        //sync_cout << "Skipping... position_index=" << position_index << " global_num_processed_positions=" << global_num_processed_positions << sync_endl;
                        continue;
                    }
                    //sync_cout << "Processing... position_index=" << position_index << " global_num_processed_positions=" << global_num_processed_positions << sync_endl;

                    // bookに格納されている値は上位16ビットが削られている
                    // このタイミングで復元する
                    Move best_move = pos.move16_to_move(book_move.bestMove);
                    if (!pos.pseudo_legal(best_move) || !pos.legal(best_move)) {
                        continue;
                    }

                    pos.do_move(best_move, state_info[1]);

                    if (pos.is_mated()) {
                        pos.undo_move(best_move);
                        continue;
                    }

                    auto value_pv = Learner::search(pos, search_depth);

                    pos.undo_move(best_move);

                    book_move.value = -value_pv.first;
                    scored = true;
                }

                if (!scored) {
                    //sync_cout << "Skipped position_index=" << position_index << " global_num_processed_positions=" << global_num_processed_positions << sync_endl;
                    continue;
                }

                int num_processed_positions = ++global_num_processed_positions;
                progress_report.Show(num_processed_positions);

                time_t now = std::time(nullptr);
                if ((num_processed_positions && num_processed_positions % save_per_positions == 0) ||
                    now - last_save_time > kSaveEvalAtMostSec) {
                    std::lock_guard<std::mutex> lock(output_book_mutex);
                    // あるスレッドが定跡データベースを保存中に他のスレッドがここを通った時、
                    // last_save_timeが古いままのため時刻チェックに失敗し、
                    // 余計に保存ルーチンが回る場合がある
                    // そのため2重にチェックする
                    if ((num_processed_positions && num_processed_positions % save_per_positions == 0) ||
                        now - last_save_time > kSaveEvalAtMostSec) {
                        sync_cout << "position_index=" << position_index <<
                            " num_processed_positions=" << num_processed_positions <<
                            " now=" << now <<
                            " last_save_time=" << last_save_time << sync_endl;
                        sync_cout << "Writing the book file..." << sync_endl;
                        // 上書き保存する
                        book.write_book(book_file, false);
                        last_save_time = now;
                        sync_cout << "done..." << sync_endl;
                    }
                }
            }
        }));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 上書き保存する
    sync_cout << "Writing the book file..." << sync_endl;
    book.write_book(book_file, false);
    sync_cout << "done..." << sync_endl;

    return true;
}

bool Book::MergeBook() {
    std::string book_file1 = Options[kBookFile];
    std::string book_file2 = Options[kBookInputFile];
    std::string book_output_file = Options[kBookOutputFile];

    sync_cout << "info string book_file1=" << book_file1 << sync_endl;
    sync_cout << "info string book_file2=" << book_file2 << sync_endl;
    sync_cout << "info string book_output_file=" << book_output_file << sync_endl;

    MemoryBook book1;
    sync_cout << "Reading book file: " << book_file1 << sync_endl;
    book1.read_book("book/" + book_file1);
    sync_cout << "done..." << sync_endl;
    sync_cout << "|book1|=" << book1.book_body.size() << sync_endl;

    MemoryBook book2;
    sync_cout << "Reading book file: " << book_file2 << sync_endl;
    book2.read_book("book/" + book_file2);
    sync_cout << "done..." << sync_endl;
    sync_cout << "|book2|=" << book2.book_body.size() << sync_endl;

    for (const auto& it2 : book2.book_body) {
        auto it1 = book1.book_body.find(it2.first);
        if (it1 == book1.book_body.end()) {
            // book1に存在しない場合は局面ごと追加する
            book1.book_body.insert(it2);
            continue;
        }

        // 各指し手をマージする
        for (const auto& it2move : *it2.second) {
            auto it1move = std::find_if(it1->second->begin(), it1->second->end(),
                [&it2move](const auto& move) {
                return move.bestMove == it2move.bestMove;
            });
            if (it1move == it1->second->end()) {
                // book1に存在しない場合は指し手を追加する
                it1->second->push_back(it2move);
                continue;
            }

            if (it1move->nextMove == Move::MOVE_NONE && it2move.nextMove != Move::MOVE_NONE) {
                // ponderが登録されていない場合はbook2の指し手の情報を追加する
                it1move->nextMove = it2move.nextMove;
            }
            it1move->num += it2move.num;
        }
    }


    // 上書き保存する
    sync_cout << "Writing the book file..." << sync_endl;
    book1.write_book("book/" + book_output_file, false);
    sync_cout << "done..." << sync_endl;

    return true;
}

bool Book::ExtendBook() {
    Search::LimitsType limits;
    // 引き分けの手数付近で引き分けの値が返るのを防ぐため1 << 16にする
    limits.max_game_ply = 1 << 16;
    limits.depth = MAX_PLY;
    limits.silent = true;
    limits.enteringKingRule = EKR_27_POINT;
    Search::Limits = limits;

    int num_threads = (int)Options[kThreads];
    std::string input_file = Options[kBookInputFile];
    std::string output_file = Options[kBookOutputFile];
    int search_depth = (int)Options[kBookSearchDepth];
    int save_per_positions = (int)Options[kBookSavePerPositions];

    // スレッド0版を未展開のノードを探すために、
    // それ以外のスレッドを未展開のノードの指し手を決定するために使うため
    // 2スレッド以上必要となる。
    ASSERT_LV3(num_threads >= 2);

    sync_cout << "info string num_threads=" << num_threads << sync_endl;
    sync_cout << "info string input_file=" << input_file << sync_endl;
    sync_cout << "info string output_file=" << output_file << sync_endl;
    sync_cout << "info string search_depth=" << search_depth << sync_endl;
    sync_cout << "info string save_per_positions=" << save_per_positions << sync_endl;

    BookMoveSelector input_book;
    sync_cout << "Reading input book file: " << input_file << sync_endl;
    input_book.memory_book.read_book("book/" + input_file);
    sync_cout << "done..." << sync_endl;
    sync_cout << "|input_book|=" << input_book.memory_book.book_body.size() << sync_endl;

    BookMoveSelector output_book;
    sync_cout << "Reading output book file: " << output_file << sync_endl;
    output_book.memory_book.read_book("book/" + output_file);
    sync_cout << "done..." << sync_endl;
    sync_cout << "|output_book|=" << output_book.memory_book.book_body.size() << sync_endl;

    time_t last_save_time = 0;
    std::time(&last_save_time);

    std::mutex sfens_mutex;
    std::mutex output_mutex;
    std::condition_variable cv;

    std::vector<std::thread> threads;
    std::set<std::string> sfens_to_be_processed;
    std::set<std::string> sfens_processing;
    // スレッド0番は未展開のノードを探すために使う
    // スレッド1番以降を未展開のノードの指し手を決定するために使う
    for (int thread_index = 1; thread_index < num_threads; ++thread_index) {
        threads.push_back(std::thread([
            thread_index, search_depth, output_file, save_per_positions,
                &last_save_time, &sfens_to_be_processed, &sfens_processing,
                &input_book, &output_book, &sfens_mutex, &output_mutex, &cv]() {
            WinProcGroup::bindThisThread(thread_index);

            for (;;) {
                std::string sfen;
                {
                    std::unique_lock<std::mutex> lock(sfens_mutex);
                    cv.wait(lock, [&sfens_to_be_processed] {
                        return !sfens_to_be_processed.empty();
                    });
                    sfen = *sfens_to_be_processed.begin();
                    sfens_to_be_processed.erase(sfens_to_be_processed.begin());
                    sfens_processing.insert(sfen);
                }

                Position& pos = Threads[thread_index]->rootPos;
                StateInfo state_info[512];
                pos.set(sfen, &state_info[0], Threads[thread_index]);

                if (pos.is_mated()) {
                    std::lock_guard<std::mutex> lock_sfens(sfens_mutex);
                    sfens_processing.erase(sfen);

                    std::lock_guard<std::mutex> lock_output(output_mutex);
                    output_book.memory_book.insert(sfen, { Move::MOVE_RESIGN, Move::MOVE_NONE, -VALUE_MATE, search_depth, 1 });
                }

                auto value_and_pv = Learner::search(pos, search_depth);

                std::lock_guard<std::mutex> lock_sfens(sfens_mutex);
                sfens_processing.erase(sfen);

                std::lock_guard<std::mutex> lock_output(output_mutex);
                Move best_move = value_and_pv.second.empty() ? Move::MOVE_NONE : value_and_pv.second[0];
                Move next_move = value_and_pv.second.size() < 2 ? Move::MOVE_NONE : value_and_pv.second[1];
                output_book.memory_book.insert(sfen, { best_move, next_move, value_and_pv.first, search_depth, 1 });

                sync_cout << "|output_book|=" << output_book.memory_book.book_body.size() <<
                    " |sfens_to_be_processed|=" << sfens_to_be_processed.size() <<
                    " |sfens_processing|=" << sfens_processing.size() << sync_endl;

                time_t now = std::time(nullptr);
                if (now - last_save_time < kSaveEvalAtMostSec) {
                    continue;
                }

                sync_cout << "Writing the book file..." << sync_endl;
                // 上書き保存する
                output_book.memory_book.write_book("book/" + output_file, false);
                last_save_time = now;
                sync_cout << "done..." << sync_endl;
            }
        }));
    }

    bool input_is_first = true;
    for (;;) {
        Position& pos = Threads[0]->rootPos;
        StateInfo state_info[512] = { 0 };
        pos.set_hirate(&state_info[0], Threads[0]);
        bool input_is_turn = input_is_first;

        for (;;) {
            if (input_is_turn) {
                input_is_turn = !input_is_turn;
                Move move = input_book.probe(pos);
                if (move != Move::MOVE_NONE && pos.pseudo_legal(move) && pos.legal(move)) {
                    pos.do_move(move, state_info[pos.game_ply()]);
                    continue;
                }

                // 入力側の定跡が外れたので終了する
                break;
            }
            else {
                input_is_turn = !input_is_turn;
                Move move = output_book.probe(pos);
                if (move != Move::MOVE_NONE && pos.pseudo_legal(move) && pos.legal(move)) {
                    pos.do_move(move, state_info[pos.game_ply()]);
                    continue;
                }

                std::string sfen = pos.sfen();
                std::lock_guard<std::mutex> lock(sfens_mutex);
                if (sfens_to_be_processed.find(sfen) != sfens_to_be_processed.end()) {
                    // キューに入っている場合もスキップする
                    //sync_cout << "in queue" << sync_endl;
                    break;
                }

                if (sfens_processing.find(sfen) != sfens_processing.end()) {
                    // 処理中の場合はスキップする
                    //sync_cout << "in processing" << sync_endl;
                    break;
                }

                // キューに入れる
                sync_cout << sfen << sync_endl;
                sfens_to_be_processed.insert(sfen);
                cv.notify_all();
                break;
            }
        }

        input_is_first = !input_is_first;
    }

    return true;
}

#endif
