//#pragma optimize( "", off )

#include "tanuki_book.h"
#include "config.h"

#ifdef EVAL_LEARN

#include <atomic>
#include <fstream>
#include <sstream>
#include <set>
#include <ctime>

#include "evaluate.h"
#include "extra/book/book.h"
#include "learn/learn.h"
#include "misc.h"
#include "position.h"
#include "tanuki_progress_report.h"
#include "thread.h"

using Book::BookMoveSelector;
using Book::BookPos;
using Book::MemoryBook;
using USI::Option;

namespace {
	constexpr const char* kBookSfenFile = "BookSfenFile";
	constexpr const char* kBookMaxMoves = "BookMaxMoves";
	constexpr const char* kBookFile = "BookFile";
	constexpr const char* kBookSearchDepth = "BookSearchDepth";
	constexpr const char* kBookSearchNodes = "BookSearchNodes";
	constexpr const char* kBookInputFile = "BookInputFile";
	constexpr const char* kBookOutputFile = "BookOutputFile";
	constexpr const char* kBookSavePerPositions = "BookSavePerPositions";
	constexpr const char* kThreads = "Threads";
	constexpr const char* kMultiPV = "MultiPV";
	constexpr const char* kBookOverwriteExistingPositions = "OverwriteExistingPositions";
	constexpr const char* kBookNarrowBook = "NarrowBook";
	constexpr int kShowProgressAtMostSec = 1 * 60;	// 1��
	//constexpr int kShowProgressAtMostSec = 60 * 60;	// 1����
	constexpr int kSaveEvalAtMostSec = 60 * 60;		// 1����
}

bool Tanuki::InitializeBook(USI::OptionsMap& o) {
	o[kBookSfenFile] << Option("merged.sfen");
	o[kBookMaxMoves] << Option(32, 0, 256);
	o[kBookSearchDepth] << Option(64, 0, 256);
	o[kBookSearchNodes] << Option(500000 * 60, 0, INT_MAX);
	o[kBookInputFile] << Option("user_book1.db");
	o[kBookOutputFile] << Option("user_book2.db");
	o[kBookSavePerPositions] << Option(1000, 1, std::numeric_limits<int>::max());
	o[kBookOverwriteExistingPositions] << Option(false);
	return true;
}

// sfen�����t�@�C�������Ճf�[�^�x�[�X���쐬����
bool Tanuki::CreateRawBook() {
	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
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

	MemoryBook memory_book;
	StateInfo state_info[4096] = {};
	StateInfo* state = state_info + 8;
	int num_records = 0;
	while (std::getline(ifs, line)) {
		std::istringstream iss(line);
		Position pos;
		pos.set_hirate(state, Threads[0]);

		std::string token;
		int num_moves = 0;
		while (num_moves < max_moves && iss >> token) {
			if (token == "startpos" || token == "moves") {
				continue;
			}
			Move move = USI::to_move(pos, token);
			if (!pos.pseudo_legal(move) || !pos.legal(move)) {
				continue;
			}

			BookPos book_pos(move, Move::MOVE_NONE, 0, 0, 1);
			memory_book.insert(pos.sfen(), book_pos);

			pos.do_move(move, state[num_moves]);
			++num_moves;
		}

		++num_records;
		if (num_records % 10000 == 0) {
			sync_cout << "info string " << num_records << sync_endl;
		}
	}

	sync_cout << "info string Writing book file..." << sync_endl;

	std::string book_file = Options[kBookFile];
	book_file = "book/" + book_file;
	memory_book.write_book(book_file);

	sync_cout << "info string Done..." << sync_endl;

	return true;
}

bool Tanuki::CreateScoredBook() {
	int num_threads = (int)Options[kThreads];
	std::string input_book_file = Options[kBookInputFile];
	int search_depth = (int)Options[kBookSearchDepth];
	int search_nodes = (int)Options[kBookSearchNodes];
	int multi_pv = (int)Options[kMultiPV];
	std::string output_book_file = Options[kBookOutputFile];
	int save_per_positions = (int)Options[kBookSavePerPositions];
	bool overwrite_existing_positions = static_cast<bool>(Options[kBookOverwriteExistingPositions]);

	sync_cout << "info string num_threads=" << num_threads << sync_endl;
	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string search_depth=" << search_depth << sync_endl;
	sync_cout << "info string search_nodes=" << search_nodes << sync_endl;
	sync_cout << "info string multi_pv=" << multi_pv << sync_endl;
	sync_cout << "info string output_book_file=" << output_book_file << sync_endl;
	sync_cout << "info string save_per_positions=" << save_per_positions << sync_endl;
	sync_cout << "info string overwrite_existing_positions=" << overwrite_existing_positions
		<< sync_endl;

	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
	limits.max_game_ply = 1 << 16;
	limits.depth = MAX_PLY;
	limits.silent = true;
	limits.enteringKingRule = EKR_27_POINT;
	Search::Limits = limits;

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
	int num_sfens = sfens.size();
	sync_cout << "Number of the positions to be processed: " << num_sfens << sync_endl;

	time_t start_time = 0;
	std::time(&start_time);

	std::atomic_int global_position_index;
	global_position_index = 0;
	std::mutex output_book_mutex;
	ProgressReport progress_report(num_sfens, kShowProgressAtMostSec);

	std::vector<std::thread> threads;
	std::atomic_int global_pos_index;
	global_pos_index = 0;
	std::atomic_int global_num_processed_positions;
	global_num_processed_positions = 0;
	for (int thread_index = 0; thread_index < num_threads; ++thread_index) {
		threads.push_back(std::thread([thread_index, num_sfens, search_depth, multi_pv, search_nodes,
			save_per_positions, &global_pos_index, &sfens, &output_book,
			&output_book_mutex, &progress_report, &output_book_file,
			&global_num_processed_positions]() {
				WinProcGroup::bindThisThread(thread_index);

				for (int position_index = global_pos_index++; position_index < num_sfens;
					position_index = global_pos_index++) {
					const std::string& sfen = sfens[position_index];
					Thread& thread = *Threads[thread_index];
					StateInfo state_info = {};
					Position& pos = thread.rootPos;
					pos.set(sfen, &state_info, &thread);

					if (pos.is_mated()) {
						continue;
					}

					Learner::search(pos, search_depth, multi_pv, search_nodes);

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

					if (num_processed_positions && num_processed_positions % save_per_positions == 0) {
						std::lock_guard<std::mutex> lock(output_book_mutex);
						sync_cout << "Writing the book file..." << sync_endl;
						output_book.write_book(output_book_file);
						sync_cout << "done..." << sync_endl;
					}
				}
			}));
	}

	for (auto& thread : threads) {
		thread.join();
	}

	sync_cout << "Writing the book file..." << sync_endl;
	output_book.write_book(output_book_file);
	sync_cout << "done..." << sync_endl;

	return true;
}

bool Tanuki::ExtendBook() {
	int num_threads = (int)Options[kThreads];
	std::string input_file = Options[kBookInputFile];
	std::string output_file = Options[kBookOutputFile];
	int search_depth = (int)Options[kBookSearchDepth];
	int search_nodes = (int)Options[kBookSearchNodes];
	int save_per_positions = (int)Options[kBookSavePerPositions];

	// �X���b�h0�ł𖢓W�J�̃m�[�h��T�����߂ɁA
	// ����ȊO�̃X���b�h�𖢓W�J�̃m�[�h�̎w��������肷�邽�߂Ɏg������
	// 2�X���b�h�ȏ�K�v�ƂȂ�B
	ASSERT_LV3(num_threads >= 2);

	sync_cout << "info string num_threads=" << num_threads << sync_endl;
	sync_cout << "info string input_file=" << input_file << sync_endl;
	sync_cout << "info string output_file=" << output_file << sync_endl;
	sync_cout << "info string search_depth=" << search_depth << sync_endl;
	sync_cout << "info string search_nodes=" << search_nodes << sync_endl;
	sync_cout << "info string save_per_positions=" << save_per_positions << sync_endl;

	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
	limits.max_game_ply = 1 << 16;
	limits.depth = MAX_PLY;
	limits.silent = true;
	limits.enteringKingRule = EKR_27_POINT;
	Search::Limits = limits;

	BookMoveSelector input_book;
	sync_cout << "Reading input book file: " << input_file << sync_endl;
	input_book.GetMemoryBook().read_book("book/" + input_file);
	sync_cout << "done..." << sync_endl;
	sync_cout << "|input_book|=" << input_book.GetMemoryBook().book_body.size() << sync_endl;

	BookMoveSelector output_book;
	sync_cout << "Reading output book file: " << output_file << sync_endl;
	output_book.GetMemoryBook().read_book("book/" + output_file);
	sync_cout << "done..." << sync_endl;
	sync_cout << "|output_book|=" << output_book.GetMemoryBook().book_body.size() << sync_endl;

	time_t last_save_time = 0;
	std::time(&last_save_time);

	std::mutex sfens_mutex;
	std::mutex output_mutex;
	std::condition_variable cv;

	std::vector<std::thread> threads;
	std::set<std::string> sfens_to_be_processed;
	std::set<std::string> sfens_processing;
	// �X���b�h0�Ԃ͖��W�J�̃m�[�h��T�����߂Ɏg��
	// �X���b�h1�Ԉȍ~�𖢓W�J�̃m�[�h�̎w��������肷�邽�߂Ɏg��
	for (int thread_index = 1; thread_index < num_threads; ++thread_index) {
		threads.push_back(std::thread([thread_index, search_depth, output_file, save_per_positions,
			&last_save_time, &sfens_to_be_processed, &sfens_processing,
			&input_book, &output_book, &sfens_mutex, &output_mutex,
			&cv]() {
				WinProcGroup::bindThisThread(thread_index);

				for (;;) {
					std::string sfen;
					{
						std::unique_lock<std::mutex> lock(sfens_mutex);
						cv.wait(lock,
							[&sfens_to_be_processed] { return !sfens_to_be_processed.empty(); });
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
						output_book.GetMemoryBook().insert(
							sfen, { Move::MOVE_RESIGN, Move::MOVE_NONE, -VALUE_MATE, search_depth, 1 });
					}

					auto value_and_pv = Learner::search(pos, search_depth);

					std::lock_guard<std::mutex> lock_sfens(sfens_mutex);
					sfens_processing.erase(sfen);

					std::lock_guard<std::mutex> lock_output(output_mutex);
					Move best_move =
						value_and_pv.second.empty() ? Move::MOVE_NONE : value_and_pv.second[0];
					Move next_move =
						value_and_pv.second.size() < 2 ? Move::MOVE_NONE : value_and_pv.second[1];
					output_book.GetMemoryBook().insert(
						sfen, Book::BookPos(best_move, next_move, value_and_pv.first, search_depth, 1));

					sync_cout << "|output_book|=" << output_book.GetMemoryBook().book_body.size()
						<< " |sfens_to_be_processed|=" << sfens_to_be_processed.size()
						<< " |sfens_processing|=" << sfens_processing.size() << sync_endl;

					time_t now = std::time(nullptr);
					if (now - last_save_time < kSaveEvalAtMostSec) {
						continue;
					}

					sync_cout << "Writing the book file..." << sync_endl;
					// �㏑���ۑ�����
					output_book.GetMemoryBook().write_book("book/" + output_file);
					last_save_time = now;
					sync_cout << "done..." << sync_endl;
				}
			}));
	}

	bool input_is_first = true;
	for (;;) {
		Position& pos = Threads[0]->rootPos;
		StateInfo state_info[512] = {};
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

				// ���͑��̒�Ղ��O�ꂽ�̂ŏI������
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
					// �L���[�ɓ����Ă���ꍇ���X�L�b�v����
					// sync_cout << "in queue" << sync_endl;
					break;
				}

				if (sfens_processing.find(sfen) != sfens_processing.end()) {
					// �������̏ꍇ�̓X�L�b�v����
					// sync_cout << "in processing" << sync_endl;
					break;
				}

				// �L���[�ɓ����
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

// �����̒�Ղ��}�[�W����
// BookInputFile�ɂ́u;�v��؂�Œ�Ճf�[�^�x�[�X�̌Ãp�[�X���w�肷��
// BookOutputFile�ɂ�book�ȉ��̃t�@�C�������w�肷��
bool Tanuki::MergeBook() {
	std::string input_file_list = Options[kBookInputFile];
	std::string output_file = Options[kBookOutputFile];

	sync_cout << "info string input_file_list=" << input_file_list << sync_endl;
	sync_cout << "info string output_file=" << output_file << sync_endl;

	BookMoveSelector output_book;
	sync_cout << "Reading output book file: " << output_file << sync_endl;
	output_book.GetMemoryBook().read_book("book/" + output_file);
	sync_cout << "done..." << sync_endl;
	sync_cout << "|output_book|=" << output_book.GetMemoryBook().book_body.size() << sync_endl;

	std::istringstream iss(input_file_list);
	std::string input_file;
	while (std::getline(iss, input_file, ';')) {
		BookMoveSelector input_book;
		sync_cout << "Reading input book file: " << input_file << sync_endl;
		input_book.GetMemoryBook().read_book(input_file);
		sync_cout << "done..." << sync_endl;
		sync_cout << "|input_book|=" << input_book.GetMemoryBook().book_body.size() << sync_endl;

		for (const auto& book_type : input_book.GetMemoryBook().book_body) {
			const auto& sfen = book_type.first;
			const auto& pos_move_list = book_type.second;

			uint64_t max_num = 0;
			for (const auto& pos_move : *pos_move_list) {
				max_num = std::max(max_num, pos_move.num);
			}

			for (const auto& pos_move : *pos_move_list) {
				if (max_num > 0 && pos_move.num == 0) {
					// �̑��񐔂��ݒ肳��Ă���A���̎�̍̑��񐔂�0�̏ꍇ�A
					// �蓮�ł��̎���w���Ȃ��悤��������Ă���B
					// ���̂悤�Ȏ�̓X�L�b�v����B
					continue;
				}
				output_book.GetMemoryBook().insert(sfen, pos_move);
			}
		}
	}

	sync_cout << "Writing the book file..." << sync_endl;
	output_book.GetMemoryBook().write_book("book/" + output_file);
	sync_cout << "done..." << sync_endl;

	return true;
}

// ��Ղ̊e�w����ɕ]���l��t����
// ����̉��肪�ݒ肳��Ă��Ȃ��ꍇ�A�ǂ݋؂���ݒ肷��B
bool Tanuki::SetScoreToMove() {
	int num_threads = (int)Options[kThreads];
	std::string input_book_file = Options[kBookInputFile];

	int search_depth = (int)Options[kBookSearchDepth];
	int search_nodes = (int)Options[kBookSearchNodes];
	std::string output_book_file = Options[kBookOutputFile];
	int save_per_positions = (int)Options[kBookSavePerPositions];
	bool overwrite_existing_positions = static_cast<bool>(Options[kBookOverwriteExistingPositions]);

	sync_cout << "info string num_threads=" << num_threads << sync_endl;
	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string search_depth=" << search_depth << sync_endl;
	sync_cout << "info string search_nodes=" << search_nodes << sync_endl;
	sync_cout << "info string output_book_file=" << output_book_file << sync_endl;
	sync_cout << "info string save_per_positions=" << save_per_positions << sync_endl;
	sync_cout << "info string overwrite_existing_positions=" << overwrite_existing_positions
		<< sync_endl;

	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
	limits.max_game_ply = 1 << 16;
	limits.depth = MAX_PLY;
	limits.silent = true;
	limits.enteringKingRule = EKR_27_POINT;
	Search::Limits = limits;

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

	// ���ɒT�������ǖʂɂ��Ă͒T�����Ȃ�
	for (const auto& sfen_and_pos_move_list : input_book.book_body) {
		if (!overwrite_existing_positions &&
			output_book.book_body.find(sfen_and_pos_move_list.first) != output_book.book_body.end()) {
			continue;
		}
		sfens.push_back(sfen_and_pos_move_list.first);
	}
	int num_sfens = sfens.size();
	sync_cout << "Number of the positions to be processed: " << num_sfens << sync_endl;

	// ��Օۑ����������̂���
	time_t start_time = 0;
	std::time(&start_time);

	// �}���`�X���b�h���������̂��߁A�C���f�b�N�X������������
	std::atomic_int global_position_index;
	global_position_index = 0;
	std::mutex output_book_mutex;

	// �i���󋵕\���̏���
	ProgressReport progress_report(num_sfens, kShowProgressAtMostSec);

	// Apery���}���`�X���b�h����
	std::vector<std::thread> threads;
	std::atomic_int global_pos_index;
	global_pos_index = 0;
	std::atomic_int global_num_processed_positions;
	global_num_processed_positions = 0;
	for (int thread_index = 0; thread_index < num_threads; ++thread_index) {
		auto procedure = [thread_index, num_sfens, search_depth, search_nodes,
			save_per_positions, &global_pos_index, &sfens, &output_book, &output_book_mutex,
			&progress_report, &output_book_file,
			&global_num_processed_positions, &input_book]() {
			WinProcGroup::bindThisThread(thread_index);

			for (int position_index = global_pos_index++; position_index < num_sfens;
				position_index = global_pos_index++) {
				const std::string& sfen = sfens[position_index];
				Thread& thread = *Threads[thread_index];
				StateInfo state_info = {};
				Position& pos = thread.rootPos;
				pos.set(sfen, &state_info, &thread);

				if (pos.is_mated()) {
					continue;
				}

				auto sfen_and_pos_move_list_ptr = input_book.book_body.find(sfen);
				if (sfen_and_pos_move_list_ptr == input_book.book_body.end()) {
					sync_cout << "sfen not found. sfen=" << sfen << sync_endl;
					continue;
				}

				for (auto& pos_move : *sfen_and_pos_move_list_ptr->second) {
					// ��Ճf�[�^�x�[�X�Ɋ܂܂�Ă���move��16�r�b�g�̂��߁A
					// ���̃^�C�~���O��32�r�b�g�ɕϊ�����B
					pos_move.bestMove = pos.move16_to_move(pos_move.bestMove);

					if (!pos.pseudo_legal(pos_move.bestMove) || !pos.legal(pos_move.bestMove)) {
						sync_cout << "Illegal move. sfen=" << sfen << " move=" << pos_move.bestMove << sync_endl;
						continue;
					}

					StateInfo state_info0;
					pos.do_move(pos_move.bestMove, state_info0);
					Eval::evaluate_with_no_return(pos);

					// ���̋ǖʂɂ��ĒT������
					auto value_and_pv = Learner::search(pos, search_depth, 1, search_nodes);

					auto value = value_and_pv.first;
					auto pv = value_and_pv.second;

					if (pos_move.nextMove == Move::MOVE_NONE && pv.size() >= 1) {
						// ��Ղ̎���w�������̋ǖʂȂ̂ŁAnextMove�ɂ�pv[0]��������B
						// �������A���Ƃ���nextMove���ݒ肳��Ă���ꍇ�A�����D�悷��B
						pos_move.nextMove = pv[0];
					}

					// �ЂƂO�̋ǖʂ��猩���]���l��������K�v������̂ŁA�����𔽓]����B
					pos_move.value = -value;

					pos_move.depth = pos.this_thread()->completedDepth;

					// ��Ղ̋ǖʂɖ߂�
					pos.undo_move(pos_move.bestMove);
				}

				// �o�͐�̒�Ղɓo�^����
				// 1�肸�o�^����ƒ��r���[�ȃ^�C�~���O�ŃX�g���[�W�ɏ������ꍇ������A
				// ���f�����ۂɈꕔ�̎肪����������Ȃ��ꍇ������B
				// ���̂��߁A�܂Ƃ߂ēo�^����
				{
					std::lock_guard<std::mutex> lock(output_book_mutex);
					for (auto& pos_move : *sfen_and_pos_move_list_ptr->second) {
						output_book.insert(sfen, pos_move);
					}
				}

				// �i���󋵂�\������
				int num_processed_positions = ++global_num_processed_positions;
				progress_report.Show(num_processed_positions);

				// ��Ղ��X�g���[�W�ɏ����o���B
				if (num_processed_positions && num_processed_positions % save_per_positions == 0) {
					std::lock_guard<std::mutex> lock(output_book_mutex);
					sync_cout << "Writing the book file..." << sync_endl;
					output_book.write_book(output_book_file);
					sync_cout << "done..." << sync_endl;
				}
			}
		};
		threads.push_back(std::thread(procedure));
	}

	for (auto& thread : threads) {
		thread.join();
	}

	sync_cout << "Writing the book file..." << sync_endl;
	output_book.write_book(output_book_file);
	sync_cout << "done..." << sync_endl;

	return true;
}

namespace {
	struct ValueMoveDepth {
		// �]���l
		Value value = VALUE_NONE;
		// ���̋ǖʂɂ�����w����
		// ��Ղ̎w����ɑ΂��鉞����Ճf�[�^�x�[�X�ɓo�^���邽�߁A�w������Ԃ���悤�ɂ��Ă���
		Move move = MOVE_NONE;
		// �T���[��
		Depth depth = DEPTH_NONE;
	};

	// (sfen, ValueAndDepth)
	// ��˂��牤�{�Ƃł͎萔�̕������폜���Ă��邪�A
	// ������e�Ղɂ��邽�ߎ萔���܂߂Ċi�[����B
	// �{���ł���΂���ɂ�镛��p���n�l����K�v�����邪�A
	// �悭������Ȃ������̂œK���Ɏ�������B
	std::unordered_map<std::string, ValueMoveDepth> memo;

	// Nega-Max�@�Ŗ��[�ǖʂ̕]���l��root�ǖʂɌ����ē`������
	// book ��Ճf�[�^�x�[�X
	// pos ���݂̋ǖ�
	// value_and_depth_without_nega_max_parent SetScoreToMove()�Őݒ肵���]���l�ƒT���[��
	//                                         ���ǖʂ��猩���]���l�ɂȂ�悤�A�����𔽓]���Ă���n������
	ValueMoveDepth NegaMax(MemoryBook& book, Position& pos, const ValueMoveDepth vmd_without_nega_max_parent, int& counter) {
		if (++counter % 100000 == 0) {
			sync_cout << counter << " |memo|=" << memo.size() << sync_endl;
		}

		auto book_moves = book.find(pos);

		if (!book_moves) {
			// ���̋ǖʂ���Ճf�[�^�x�[�X�ɓo�^����Ă��Ȃ�=���[�ǖʂł���B
			// SetScoreToMove()�ɂ��T���̌��ʂ�Ԃ��B
			return vmd_without_nega_max_parent;
		}

		std::string sfen = pos.sfen();
		auto& vmd = memo[sfen];
		if (vmd.depth != DEPTH_NONE) {
			// �L���b�V���Ƀq�b�g�����ꍇ�A���̒l��Ԃ��B
			return vmd;
		}

		if (pos.is_mated()) {
			// �l��ł���ꍇ
			vmd.value = mated_in(0);
			vmd.depth = DEPTH_ZERO;
			return vmd;
		}

		if (pos.DeclarationWin() != MOVE_NONE) {
			// �錾�����ł���ꍇ
			vmd.value = mate_in(1);
			vmd.depth = DEPTH_ZERO;
			return vmd;
		}

		auto repetition_state = pos.is_repetition();
		switch (repetition_state) {
		case REPETITION_WIN:
			// �A������̐����ɂ�艿�l�̏ꍇ
			vmd.value = mate_in(MAX_PLY);
			vmd.depth = DEPTH_ZERO;
			return vmd;

		case REPETITION_LOSE:
			// �A������̐����ɂ�蕉���̏ꍇ
			vmd.value = mated_in(MAX_PLY);
			vmd.depth = DEPTH_ZERO;
			return vmd;

		case REPETITION_DRAW:
			// ���������̏ꍇ
			vmd.value = static_cast<Value>(Options["Contempt"] * Eval::PawnValue / 100);
			vmd.depth = DEPTH_ZERO;
			return vmd;

		case REPETITION_SUPERIOR:
			// �D���ǖʂ̏ꍇ
			vmd.value = VALUE_SUPERIOR;
			vmd.depth = DEPTH_ZERO;
			return vmd;

		case REPETITION_INFERIOR:
			// �򓙋ǖʂ̏ꍇ
			vmd.value = -VALUE_SUPERIOR;
			vmd.depth = DEPTH_ZERO;
			return vmd;

		default:
			break;
		}

		vmd.value = mated_in(0);
		vmd.depth = DEPTH_ZERO;
		// �S���@��ɂ��Ē��ׂ�
		for (const auto& move : MoveList<LEGAL_ALL>(pos)) {
			auto book_move = std::find_if(book_moves->begin(), book_moves->end(),
				[&move, &pos](const BookPos& book_move) {
					// ��Ճf�[�^�x�[�X�Ɋ܂܂�Ă���move��16�r�b�g�̂��߁A32�r�b�g�ɕϊ�����B
					Move move32 = pos.move16_to_move(book_move.bestMove);
					return move.move == move32;
				});

			// ��Ճf�[�^�x�[�X�̎w����ɓo�^����Ă���]���l�ƒT���[����
			// �q�ǖʂɓn�����߂̃C���X�^���X
			ValueMoveDepth vmd_without_nega_max_child;
			if (book_move != book_moves->end()) {
				// �w����ɂ����Ă���]���l�͌��ǖʂ��猩���]���l�Ȃ̂ŁA
				// NegaMax()�ɓn���Ƃ��ɕ����𔽓]����
				vmd_without_nega_max_child.value = static_cast<Value>(-book_move->value);
				vmd_without_nega_max_child.depth = static_cast<Depth>(book_move->depth);
			}

			StateInfo state_info = {};
			pos.do_move(move, state_info);
			auto value_and_depth_child = NegaMax(book, pos, vmd_without_nega_max_child, counter);
			pos.undo_move(move);

			if (vmd_without_nega_max_child.depth == DEPTH_NONE) {
				// �q�ǖʂ���Ճf�[�^�x�[�X�ɓo�^����Ă��Ȃ������ꍇ
				continue;
			}

			// �w������ɒT���̌��ʂ��i�[����
			// �Ԃ��Ă����]���l�͎��̋ǖʂ��猩���]���l�Ȃ̂ŁA�����𔽓]����
			BookPos new_book_move(move, value_and_depth_child.move, -value_and_depth_child.value,
				value_and_depth_child.depth, 1);
			book.insert(sfen, new_book_move);

			if (vmd.value < new_book_move.value) {
				vmd.value = static_cast<Value>(new_book_move.value);
				vmd.move = move;
			}
			// �ł��[���T���[��+1��ݒ肷��
			vmd.depth = std::max(vmd.depth, static_cast<Depth>(new_book_move.depth + 1));
		}

		return vmd;
	}
}

// ��Ճf�[�^�x�[�X�̖��[�ǖʂ̕]���l��root�ǖʂɌ����ē`������
bool Tanuki::TeraShock() {
	std::string input_book_file = Options[kBookInputFile];
	std::string output_book_file = Options[kBookOutputFile];

	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string output_book_file=" << output_book_file << sync_endl;

	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
	limits.max_game_ply = 1 << 16;
	limits.depth = MAX_PLY;
	limits.silent = true;
	limits.enteringKingRule = EKR_27_POINT;
	Search::Limits = limits;

	MemoryBook book;
	input_book_file = "book/" + input_book_file;
	sync_cout << "Reading input book file: " << input_book_file << sync_endl;
	book.read_book(input_book_file);
	sync_cout << "done..." << sync_endl;
	sync_cout << "|input_book_file|=" << book.book_body.size() << sync_endl;

	// ����̋ǖʂ��炽�ǂ��ǖʂɂ��ď�������
	auto& pos = Threads[0]->rootPos;
	StateInfo state_info = {};
	pos.set_hirate(&state_info, Threads[0]);
	ValueMoveDepth root_vmd = {};
	int counter = 0;
	NegaMax(book, pos, root_vmd, counter);

	// ����̋ǖʂ���H��Ȃ������ǖʂ���������
	for (const auto& book_entry : book.book_body) {
		auto& pos = Threads[0]->rootPos;
		StateInfo state_info = {};
		pos.set(book_entry.first, &state_info, Threads[0]);
		ValueMoveDepth root_vmd = {};
		NegaMax(book, pos, root_vmd, counter);
	}

	output_book_file = "book/" + output_book_file;
	sync_cout << "Writing output book file: " << output_book_file << sync_endl;
	book.write_book(output_book_file);
	sync_cout << "done..." << sync_endl;
	sync_cout << "|output_book|=" << book.book_body.size() << sync_endl;

	return true;
}

// �e���V���b�N��Ղ���������
bool Tanuki::ExtendTeraShock() {
	int num_threads = (int)Options[kThreads];
	int multi_pv = (int)Options[kMultiPV];
	std::string input_book_file = Options[kBookInputFile];
	int search_depth = (int)Options[kBookSearchDepth];
	int search_nodes = (int)Options[kBookSearchNodes];
	std::string output_book_file = Options[kBookOutputFile];
	int save_per_positions = (int)Options[kBookSavePerPositions];

	sync_cout << "info string num_threads=" << num_threads << sync_endl;
	sync_cout << "info string multi_pv=" << multi_pv << sync_endl;
	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string search_depth=" << search_depth << sync_endl;
	sync_cout << "info string search_nodes=" << search_nodes << sync_endl;
	sync_cout << "info string output_book_file=" << output_book_file << sync_endl;
	sync_cout << "info string save_per_positions=" << save_per_positions << sync_endl;

	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
	limits.max_game_ply = 1 << 16;
	limits.depth = MAX_PLY;
	limits.silent = true;
	limits.enteringKingRule = EKR_27_POINT;
	Search::Limits = limits;

	BookMoveSelector book;
	input_book_file = "book/" + input_book_file;
	sync_cout << "Reading input book file: " << input_book_file << sync_endl;
	book.GetMemoryBook().read_book(input_book_file);
	sync_cout << "done..." << sync_endl;
	sync_cout << "|input_book_file|=" << book.GetMemoryBook().book_body.size() << sync_endl;

	std::vector<std::thread> threads;
	std::set<std::string> searching_positions;
	std::mutex mutex;
	std::atomic_int global_num_processed_positions;
	global_num_processed_positions = 0;
	for (int thread_index = 0; thread_index < num_threads; ++thread_index) {
		auto procedure = [thread_index, multi_pv, search_depth, search_nodes,
			save_per_positions, &book, &mutex, &searching_positions,
			&global_num_processed_positions, &output_book_file]() {
			for (;;) {
				std::vector<StateInfo> state_info(512);
				std::vector<Move> moves;
				//int thread_index = 0;
				auto th = Threads[thread_index];
				auto& pos = th->rootPos;
				pos.set_hirate(&state_info[0], Threads[thread_index]);

				bool extend = false;
				{
					std::lock_guard lock(mutex);

					for (;;) {
						auto book_moves = book.GetMemoryBook().find(pos);

						if (!book_moves) {
							// ��Ճf�[�^�x�[�X�̖��[�ǖʂɂ��ǂ蒅�����̂ŉ�������
							extend = true;
							break;
						}

						if (book_moves->size() < multi_pv) {
							// �w����̌�␔�����Ȃ��̂ŉ�������
							extend = true;
							break;
						}

						auto move = book.probe(pos);
						if (move == MOVE_NONE) {
							// ��ԑ����猩�ĕs���ȋǖ�
							// ���̋ǖʂɂ��ǂ蒅������ǂ����悤���Ȃ��̂ŉ������Ȃ�
							break;
						}

						// ��Ղ̎w����ɏ]���Ďw�����i�߂�
						moves.push_back(move);
						pos.do_move(move, state_info[pos.game_ply()]);
					}
				}

				if (!extend) {
					continue;
				}

				//sync_cout << pos << sync_endl;

				{
					std::lock_guard lock(mutex);
					if (searching_positions.count(pos.sfen())) {
						// ���ݑ��̃X���b�h�ŒT�����̏ꍇ�͒T�����Ȃ�
						continue;
					}
					searching_positions.insert(pos.sfen());
				}

				// MultiPV�ŒT������
				Learner::search(pos, search_depth, multi_pv, search_nodes);

				{
					std::lock_guard lock(mutex);

					// ���̂ق���pos��ύX����̂ŁA���̃^�C�~���O�ŒT�����t���O�����낵�Ă����B
					searching_positions.erase(pos.sfen());

					// ��Ճf�[�^�x�[�X���X�V����
					for (int pv_index = 0; pv_index < multi_pv && pv_index < th->rootMoves.size(); ++pv_index) {
						Move best_move = MOVE_NONE;
						if (th->rootMoves[pv_index].pv.size() > 0) {
							best_move = th->rootMoves[pv_index].pv[0];
						}

						Move next_move = MOVE_NONE;
						if (th->rootMoves[pv_index].pv.size() > 1) {
							next_move = th->rootMoves[pv_index].pv[1];
						}

						int value = th->rootMoves[pv_index].selDepth;
						int depth = th->completedDepth;
						// �����̎w����͂ɂ́A��Ճc���[�T����̕]���l�������Ă���\��������̂ŁA
						// �㏑�����Ȃ��悤�ɂ���
						book.GetMemoryBook().insert(pos.sfen(), BookPos(best_move, next_move, value, depth, 1), false);
					}

					// ����̋ǖʂɌ������ĕ]���l��`������
					while (!moves.empty())
					{
						// ���̋ǖʂ̕]���l�����߂�
						auto pos_move_list = book.GetMemoryBook().find(pos);

						// ���̋ǖʂ͕K��������͂�
						ASSERT_LV3(pos_move_list);

						Value best_value = -VALUE_MATE;
						Depth best_depth = DEPTH_ZERO;
						// ���O�̋ǖʂ̎w����ɑ΂��鉞��ƂȂ�
						Move next_move = MOVE_NONE;
						for (const auto& book_move : *pos_move_list) {
							if (best_value >= book_move.value) {
								continue;
							}
							best_value = static_cast<Value>(book_move.value);
							best_depth = static_cast<Depth>(book_move.depth);
							next_move = static_cast<Move>(book_move.bestMove);
						}

						// �w����͕K���o�^����Ă���͂�
						ASSERT_LV3(best_value != -VALUE_MATE);

						// ���߂�
						Move best_move = moves.back();
						pos.undo_move(best_move);
						moves.pop_back();

						pos_move_list = book.GetMemoryBook().find(pos);

						// ���̋ǖʂ͕K��������͂�
						ASSERT_LV3(pos_move_list);

						auto book_move = std::find_if(pos_move_list->begin(), pos_move_list->end(),
							[best_move, &pos](const auto& x) {
								return pos.move16_to_move(best_move) ==
									pos.move16_to_move(x.bestMove);
							});

						// �w����͕K��������͂�
						ASSERT_LV3(book_move != pos_move_list->end());

						book_move->nextMove = next_move;
						book_move->depth = best_depth + 1;
						book_move->value = -best_value;
					}

					// ��Ղ��X�g���[�W�ɏ����o���B
					int num_processed_positions = ++global_num_processed_positions;

					if (num_processed_positions && num_processed_positions % save_per_positions == 0) {
						sync_cout << "Writing the book file..." << sync_endl;
						book.GetMemoryBook().write_book(output_book_file);
						sync_cout << "done..." << sync_endl;
					}
				}
			}
		};
		threads.push_back(std::thread(procedure));
	}

	for (auto& thread : threads) {
		thread.join();
	}

	return true;
}

#endif
