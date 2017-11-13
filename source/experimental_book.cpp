#include "experimental_book.h"

#include <atomic>
#include <fstream>
#include <sstream>

#include "extra/book.h"
#include "misc.h"
#include "position.h"
#include "progress_report.h"
#include "thread.h"

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
constexpr int kShowProgressAtMostSec = 10 * 60;
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

    StateInfo state_info[512];
    memset(state_info, 0, sizeof(state_info));
    std::map<std::string, int> sfen_to_count;
    int num_records = 0;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        Position pos;
        pos.set_hirate();
        ++sfen_to_count[pos.sfen()];

        std::string token;
        int num_moves = 0;
        while (num_moves < max_moves && iss >> token) {
            if (token == "startpos" || token == "moves") {
                continue;
            }
            Move move = move_from_usi(pos, token);
            pos.do_move(move, state_info[num_moves + 5]);
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
        insert_book_pos(memory_book, sfen, book_pos);
    }

    std::string book_file = Options[kBookFile];
    book_file = "book/" + book_file;
    write_book(book_file, memory_book, true);

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
    read_book(input_book_file, input_book);
    sync_cout << "done..." << sync_endl;
    sync_cout << "|input_book|=" << input_book.book_body.size() << sync_endl;

    MemoryBook output_book;
    output_book_file = "book/" + output_book_file;
    sync_cout << "Reading output book file: " << output_book_file << sync_endl;
    read_book(output_book_file, output_book);
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
    int num_sfens = sfens.size();
    sync_cout << "Number of the positions to be processed: " << num_sfens << sync_endl;

    time_t start_time = 0;
    std::time(&start_time);

    std::atomic_int global_position_index = 0;
    std::mutex output_book_mutex;
    ProgressReport progress_report(num_sfens, kShowProgressAtMostSec);

    std::vector<std::thread> threads;
    std::atomic_int global_pos_index = 0;
    std::atomic_int global_num_processed_positions = 0;
    for (int thread_index = 0; thread_index < num_threads; ++thread_index) {
        threads.push_back(std::thread([thread_index, num_sfens, search_depth, multi_pv,
                                       save_per_positions, &global_pos_index, &sfens, &output_book,
                                       &output_book_mutex, &progress_report, &output_book_file,
                                       &global_num_processed_positions]() {
            WinProcGroup::bindThisThread(thread_index);

            for (int position_index = global_pos_index++; position_index < num_sfens;
                 position_index = global_pos_index++) {
                const std::string& sfen = sfens[position_index];
                Thread& thread = *Threads[thread_index];
                Position& pos = thread.rootPos;
                pos.set(sfen);
                // Position::set()で内容が全てクリアされるので
                // 後からset_this_thread()を行う。
                pos.set_this_thread(&thread);

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
                        insert_book_pos(output_book, sfen, book_pos);
                    }
                }

                int num_processed_positions = ++global_num_processed_positions;
                progress_report.Show(num_processed_positions);

                if (num_processed_positions && num_processed_positions % save_per_positions == 0) {
                    std::lock_guard<std::mutex> lock(output_book_mutex);
                    sync_cout << "Writing the book file..." << sync_endl;
                    write_book(output_book_file, output_book, false);
                    sync_cout << "done..." << sync_endl;
                }
            }
        }));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    sync_cout << "Writing the book file..." << sync_endl;
    write_book(output_book_file, output_book, false);
    sync_cout << "done..." << sync_endl;

    return true;
}
