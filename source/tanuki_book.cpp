//#pragma optimize( "", off )

#include "tanuki_book.h"
#include "config.h"

#ifdef EVAL_LEARN

#include <atomic>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <queue>
#include <random>
#include <set>
#include <sstream>

#include <omp.h>

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
	constexpr const char* kThreads = "Threads";
	constexpr const char* kMultiPV = "MultiPV";
	constexpr const char* kBookOverwriteExistingPositions = "OverwriteExistingPositions";
	constexpr const char* kBookNarrowBook = "NarrowBook";
	constexpr const char* kBookTargetSfensFile = "BookTargetSfensFile";
	constexpr int kShowProgressPerAtMostSec = 1 * 60 * 60;	// 1����
	constexpr time_t kSavePerAtMostSec = 6 * 60 * 60;		// 6����

	struct SfenAndMove {
		std::string sfen;
		Move best_move;
		Move next_move;
	};

	void WriteBook(Book::MemoryBook& book, const std::string output_book_file_path) {
		std::string backup_file_path = output_book_file_path + ".bak";

		if (std::filesystem::exists(backup_file_path)) {
			sync_cout << "Removing the backup file. backup_file_path=" << backup_file_path << sync_endl;
			std::filesystem::remove(backup_file_path);
		}

		if (std::filesystem::exists(output_book_file_path)) {
			sync_cout << "Renaming the output file. output_book_file_path=" << output_book_file_path << " backup_file_path=" << backup_file_path << sync_endl;
			std::filesystem::rename(output_book_file_path, backup_file_path);
		}

		book.write_book(output_book_file_path);
		sync_cout << "|output_book_file_path|=" << book.book_body.size() << sync_endl;
	}

	void WriteBook(BookMoveSelector& book, const std::string output_book_file_path) {
		WriteBook(book.GetMemoryBook(), output_book_file_path);
	}
}

bool Tanuki::InitializeBook(USI::OptionsMap& o) {
	o[kBookSfenFile] << Option("merged.sfen");
	o[kBookMaxMoves] << Option(32, 0, 256);
	o[kBookSearchDepth] << Option(64, 0, 256);
	o[kBookSearchNodes] << Option(500000 * 60, 0, INT_MAX);
	o[kBookInputFile] << Option("user_book1.db");
	o[kBookOutputFile] << Option("user_book2.db");
	o[kBookOverwriteExistingPositions] << Option(false);
	o[kBookTargetSfensFile] << Option("");
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

	WriteBook(memory_book, "book/" + Options[kBookFile]);

	return true;
}

bool Tanuki::CreateScoredBook() {
	int num_threads = (int)Options[kThreads];
	std::string input_book_file = Options[kBookInputFile];
	int search_depth = (int)Options[kBookSearchDepth];
	int search_nodes = (int)Options[kBookSearchNodes];
	int multi_pv = (int)Options[kMultiPV];
	std::string output_book_file = Options[kBookOutputFile];
	bool overwrite_existing_positions = static_cast<bool>(Options[kBookOverwriteExistingPositions]);

	omp_set_num_threads(num_threads);

	sync_cout << "info string num_threads=" << num_threads << sync_endl;
	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string search_depth=" << search_depth << sync_endl;
	sync_cout << "info string search_nodes=" << search_nodes << sync_endl;
	sync_cout << "info string multi_pv=" << multi_pv << sync_endl;
	sync_cout << "info string output_book_file=" << output_book_file << sync_endl;
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
	ProgressReport progress_report(num_sfens, kShowProgressPerAtMostSec);
	time_t last_save_time_sec = std::time(nullptr);

	std::atomic<bool> need_wait = false;
	std::atomic_int global_pos_index;
	global_pos_index = 0;
	std::atomic_int global_num_processed_positions;
	global_num_processed_positions = 0;
#pragma omp parallel
	{
		int thread_index = ::omp_get_thread_num();
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

			// �i���󋵂�\������
			int num_processed_positions = ++global_num_processed_positions;
			progress_report.Show(num_processed_positions);

			// ��莞�Ԃ��Ƃɕۑ�����
			{
				std::lock_guard<std::mutex> lock(output_book_mutex);
				if (last_save_time_sec + kSavePerAtMostSec < std::time(nullptr)) {
					WriteBook(output_book, output_book_file);
					last_save_time_sec = std::time(nullptr);
				}
			}

			need_wait = need_wait ||
				(progress_report.HasDataPerTime() &&
					progress_report.GetDataPerTime() * 2 < progress_report.GetMaxDataPerTime());

			if (need_wait) {
				// �������x���ቺ���Ă��Ă���B
				// �S�ẴX���b�h��ҋ@����B
#pragma omp barrier

				// �}�X�^�[�X���b�h�ł��΂炭�ҋ@����B
#pragma omp master
				{
					sync_cout << "Speed is down. Waiting for a while. GetDataPerTime()=" <<
						progress_report.GetDataPerTime() << " GetMaxDataPerTime()=" <<
						progress_report.GetMaxDataPerTime() << sync_endl;

					std::this_thread::sleep_for(std::chrono::minutes(10));
					progress_report.Reset();
					need_wait = false;
				}

				// �}�X�^�[�X���b�h�̑ҋ@���I���܂ŁA�ēx�S�ẴX���b�h��ҋ@����B
#pragma omp barrier
			}
		}
	}

	WriteBook(output_book, output_book_file);

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

	std::vector<std::string> input_files;
	{
		std::istringstream iss(input_file_list);
		std::string input_file;
		while (std::getline(iss, input_file, ';')) {
			input_files.push_back(input_file);
		}
	}

	for (int input_file_index = 0; input_file_index < input_files.size(); ++input_file_index) {
		sync_cout << (input_file_index + 1) << " / " << input_files.size() << sync_endl;

		const auto& input_file = input_files[input_file_index];
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
				output_book.GetMemoryBook().insert(sfen, pos_move, false);
			}
		}
	}

	WriteBook(output_book, "book/" + output_file);

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

	omp_set_num_threads(num_threads);

	sync_cout << "info string num_threads=" << num_threads << sync_endl;
	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string search_depth=" << search_depth << sync_endl;
	sync_cout << "info string search_nodes=" << search_nodes << sync_endl;
	sync_cout << "info string output_book_file=" << output_book_file << sync_endl;

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

	std::vector<SfenAndMove> sfen_and_moves;

	// �������̋ǖʂ��L���[�ɓo�^����
	for (const auto& input_sfen_and_pos_move_list : input_book.book_body) {
		const auto& input_sfen = input_sfen_and_pos_move_list.first;

		auto output_book_moves_it = output_book.book_body.find(input_sfen);
		if (output_book_moves_it == output_book.book_body.end()) {
			// �o�͒�Ճf�[�^�x�[�X�ɋǖʂ��o�^����Ă��Ȃ������ꍇ
			for (const auto& input_book_move : *input_sfen_and_pos_move_list.second) {
				sfen_and_moves.push_back({ input_sfen, input_book_move.bestMove,
					input_book_move.nextMove });
			}
			continue;
		}

		const auto& output_book_moves = *output_book_moves_it->second;
		for (const auto& input_book_move : *input_sfen_and_pos_move_list.second) {
			if (std::find_if(output_book_moves.begin(), output_book_moves.end(),
				[&input_book_move](const auto& x) {
					return USI::move(input_book_move.bestMove) == USI::move(x.bestMove);
				}) != output_book_moves.end()) {
				continue;
			}

			// �o�͒�Ճf�[�^�x�[�X�Ɏw���肪�o�^����Ă��Ȃ������ꍇ
			sfen_and_moves.push_back({ input_sfen, input_book_move.bestMove,
				input_book_move.nextMove });
		}
	}
	int num_sfen_and_moves = sfen_and_moves.size();
	sync_cout << "Number of the moves to be processed: " << num_sfen_and_moves << sync_endl;

	// �}���`�X���b�h���������̂��߁A�C���f�b�N�X������������
	std::atomic_int global_sfen_and_move_index;
	global_sfen_and_move_index = 0;
	std::mutex output_book_mutex;

	// �i���󋵕\���̏���
	ProgressReport progress_report(num_sfen_and_moves, kShowProgressPerAtMostSec);

	// ��Ղ����I�ɕۑ����邽�߂̕ϐ�
	time_t last_save_time_sec = std::time(nullptr);

	// Apery���}���`�X���b�h����
	std::atomic<bool> need_wait = false;
	std::atomic_int global_num_processed_positions;
	global_num_processed_positions = 0;
#pragma omp parallel
	{
		int thread_index = ::omp_get_thread_num();
		WinProcGroup::bindThisThread(thread_index);

		for (int position_index = global_sfen_and_move_index++; position_index < num_sfen_and_moves;
			position_index = global_sfen_and_move_index++) {
			const auto& sfen_and_move = sfen_and_moves[position_index];
			const auto& sfen = sfen_and_move.sfen;
			Thread& thread = *Threads[thread_index];
			StateInfo state_info = {};
			Position& pos = thread.rootPos;
			pos.set(sfen, &state_info, &thread);
			Move best_move = pos.move16_to_move(sfen_and_move.best_move);
			Move next_move = MOVE_NONE;

			if (pos.is_mated()) {
				continue;
			}

			if (!pos.pseudo_legal(best_move) || !pos.legal(best_move)) {
				sync_cout << "Illegal move. sfen=" << sfen << " best_move=" <<
					USI::move(best_move) << " next_move=" << USI::move(next_move) << sync_endl;
				continue;
			}

			StateInfo state_info0;
			pos.do_move(best_move, state_info0);
			Eval::evaluate_with_no_return(pos);

			// ���̋ǖʂɂ��ĒT������
			auto value_and_pv = Learner::search(pos, search_depth, 1, search_nodes);

			// �ЂƂO�̋ǖʂ��猩���]���l��������K�v������̂ŁA�����𔽓]����B
			Value value = -value_and_pv.first;

			auto pv = value_and_pv.second;
			if (next_move == MOVE_NONE && pv.size() >= 1) {
				// ��Ղ̎���w�������̋ǖʂȂ̂ŁAnextMove�ɂ�pv[0]��������B
				// �������A���Ƃ���nextMove���ݒ肳��Ă���ꍇ�A�����D�悷��B
				next_move = pv[0];
			}

			Depth depth = pos.this_thread()->completedDepth;

			// �w������o�͐�̒�Ղɓo�^����
			{
				std::lock_guard<std::mutex> lock(output_book_mutex);
				output_book.insert(sfen, BookPos(best_move, next_move, value, depth, 1));

				// �i���󋵂�\������
				int num_processed_positions = ++global_num_processed_positions;
				progress_report.Show(num_processed_positions);

				// ��Ղ��X�g���[�W�ɏ����o���B
				if (last_save_time_sec + kSavePerAtMostSec < std::time(nullptr)) {
					WriteBook(output_book, output_book_file);
					last_save_time_sec = std::time(nullptr);
				}
			}
		}

		need_wait = need_wait ||
			(progress_report.HasDataPerTime() &&
				progress_report.GetDataPerTime() * 2 < progress_report.GetMaxDataPerTime());

		if (need_wait) {
			// �������x���ቺ���Ă��Ă���B
			// �S�ẴX���b�h��ҋ@����B
#pragma omp barrier

			// �}�X�^�[�X���b�h�ł��΂炭�ҋ@����B
#pragma omp master
			{
				sync_cout << "Speed is down. Waiting for a while. GetDataPerTime()=" <<
					progress_report.GetDataPerTime() << " GetMaxDataPerTime()=" <<
					progress_report.GetMaxDataPerTime() << sync_endl;

				std::this_thread::sleep_for(std::chrono::minutes(10));
				progress_report.Reset();
				need_wait = false;
			}

			// �}�X�^�[�X���b�h�̑ҋ@���I���܂ŁA�ēx�S�ẴX���b�h��ҋ@����B
#pragma omp barrier
		}
	}

	WriteBook(output_book, output_book_file);

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

	// IgnoreBookPly�I�v�V������true�̏ꍇ�A
	// �^����ꂽsfen�����񂩂�ply����菜��
	std::string RemovePlyIfIgnoreBookPly(const std::string& sfen) {
		if (Options["IgnoreBookPly"]) {
			return StringExtension::trim_number(sfen);
		}
		else {
			return sfen;
		}
	}

	Move To16Bit(Move move) {
		return static_cast<Move>(static_cast<int>(move) & 0xffff);
	}

	// Nega-Max�@�Ŗ��[�ǖʂ̕]���l��root�ǖʂɌ����ē`������
	// book ��Ճf�[�^�x�[�X
	// pos ���݂̋ǖ�
	// value_and_depth_without_nega_max_parent SetScoreToMove()�Őݒ肵���]���l�ƒT���[��
	//                                         ���ǖʂ��猩���]���l�ɂȂ�悤�A�����𔽓]���Ă���n������
	ValueMoveDepth NegaMax(MemoryBook& book, Position& pos, const ValueMoveDepth vmd_without_nega_max_parent, int& counter) {
		if (++counter % 1000000 == 0) {
			sync_cout << counter << " |memo|=" << memo.size() << sync_endl;
		}

		// ���̋ǖʂɑ΂��ẴL�[�Ƃ��Ďg�p����sfen������B
		// �K�v�ɉ����Ė�����ply����菜���Ă����B
		std::string sfen = RemovePlyIfIgnoreBookPly(pos.sfen());
		auto book_moves = book.book_body.find(sfen);

		if (book_moves == book.book_body.end()) {
			// ���̋ǖʂ���Ճf�[�^�x�[�X�ɓo�^����Ă��Ȃ�=���[�ǖʂł���B
			// SetScoreToMove()�ɂ��T���̌��ʂ�Ԃ��B
			return vmd_without_nega_max_parent;
		}

		auto& vmd = memo[sfen];
		if (vmd.depth != DEPTH_NONE) {
			// �L���b�V���Ƀq�b�g�����ꍇ�A���̒l��Ԃ��B
			return vmd;
		}

		if (pos.is_mated()) {
			// �l��ł���ꍇ
			vmd.value = mated_in(0);
			vmd.depth = 0;
			return vmd;
		}

		if (pos.DeclarationWin() != MOVE_NONE) {
			// �錾�����ł���ꍇ
			vmd.value = mate_in(1);
			vmd.depth = 0;
			return vmd;
		}

		auto repetition_state = pos.is_repetition();
		switch (repetition_state) {
		case REPETITION_WIN:
			// �A������̐����ɂ�艿�l�̏ꍇ
			vmd.value = mate_in(MAX_PLY);
			vmd.depth = 0;
			return vmd;

		case REPETITION_LOSE:
			// �A������̐����ɂ�蕉���̏ꍇ
			vmd.value = mated_in(MAX_PLY);
			vmd.depth = 0;
			return vmd;

		case REPETITION_DRAW:
			// ���������̏ꍇ
			vmd.value = static_cast<Value>(Options["Contempt"] * Eval::PawnValue / 100);
			vmd.depth = 0;
			return vmd;

		case REPETITION_SUPERIOR:
			// �D���ǖʂ̏ꍇ
			vmd.value = VALUE_SUPERIOR;
			vmd.depth = 0;
			return vmd;

		case REPETITION_INFERIOR:
			// �򓙋ǖʂ̏ꍇ
			vmd.value = -VALUE_SUPERIOR;
			vmd.depth = 0;
			return vmd;

		default:
			break;
		}

		vmd.value = mated_in(0);
		vmd.depth = 0;
		// �S���@��ɂ��Ē��ׂ�
		for (const auto& move : MoveList<LEGAL_ALL>(pos)) {
			auto book_move = std::find_if(book_moves->second->begin(), book_moves->second->end(),
				[&move, &pos](const BookPos& book_move) {
					// ��Ճf�[�^�x�[�X�Ɋ܂܂�Ă���move��16�r�b�g�̂��߁A32�r�b�g�ɕϊ�����B
					Move move32 = pos.move16_to_move(book_move.bestMove);
					return move.move == move32;
				});

			// ��Ճf�[�^�x�[�X�̎w����ɓo�^����Ă���]���l�ƒT���[����
			// �q�ǖʂɓn�����߂̃C���X�^���X
			ValueMoveDepth vmd_without_nega_max_child;
			if (book_move != book_moves->second->end()) {
				// �w����ɂ����Ă���]���l�͌��ǖʂ��猩���]���l�Ȃ̂ŁA
				// NegaMax()�ɓn���Ƃ��ɕ����𔽓]����
				vmd_without_nega_max_child.value = static_cast<Value>(-book_move->value);
				vmd_without_nega_max_child.depth = static_cast<Depth>(book_move->depth);
				// �q�ǖʂ̎w����Ȃ̂�nextMove��������
				vmd_without_nega_max_child.move = static_cast<Move>(book_move->nextMove);
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
			BookPos new_book_move(To16Bit(move), To16Bit(value_and_depth_child.move), -value_and_depth_child.value,
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
bool Tanuki::PropagateLeafNodeValuesToRoot() {
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

	WriteBook(book, "book/" + output_book_file);
	sync_cout << "|output_book|=" << book.book_body.size() << sync_endl;

	return true;
}

// ��Ճf�[�^�x�[�X�̊e�ǖʂ̕]���l��e�ǖʂɓ`������̂��J��Ԃ�
bool Tanuki::PropagateLeafNodeValuesToRootOne() {
	std::string input_book_file = Options[kBookInputFile];
	std::string output_book_file = Options[kBookOutputFile];
	bool ignore_book_ply = Options["IgnoreBookPly"];

	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string output_book_file=" << output_book_file << sync_endl;
	sync_cout << "info string ignore_book_ply=" << ignore_book_ply << sync_endl;

	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
	limits.max_game_ply = 1 << 16;
	limits.depth = MAX_PLY;
	limits.silent = true;
	limits.enteringKingRule = EKR_27_POINT;
	Search::Limits = limits;

	MemoryBook current_book;
	input_book_file = "book/" + input_book_file;
	sync_cout << "Reading input book file: " << input_book_file << sync_endl;
	current_book.read_book(input_book_file);
	sync_cout << "done..." << sync_endl;
	sync_cout << "|input_book_file|=" << current_book.book_body.size() << sync_endl;

	// 1�肸�`�������Ă���
	for (int depth = 1; depth <= 16; ++depth) {
		sync_cout << "depth:" << depth << sync_endl;

		// ����MemoryBook�ɏ������ʂ�ǉ����Ă���
		MemoryBook next_book;

		int counter = 0;
		for (const auto& current_sfen_and_book_moves : current_book.book_body) {
			if (++counter % 100000 == 0) {
				sync_cout << counter << sync_endl;
			}

			const auto& current_sfen = current_sfen_and_book_moves.first;
			const auto& current_book_moves = *current_sfen_and_book_moves.second;

			Position& pos = Threads[0]->rootPos;
			StateInfo state_info0;
			pos.set(current_sfen, &state_info0, Threads[0]);

			for (auto& current_book_move : current_book_moves) {
				Move best_move = pos.move16_to_move(current_book_move.bestMove);
				if (!pos.pseudo_legal(best_move) ||
					!pos.legal(best_move)) {
					continue;
				}

				StateInfo state_info1;
				pos.do_move(best_move, state_info1);

				// MemoryBook::find()��IgnoreBookPly�ɑΉ����Ă��Ȃ����߁A
				// MemoryBook::book_body::find()�𒼐ڌĂяo��
				auto current_child_book_moves = current_book.book_body.find(
					RemovePlyIfIgnoreBookPly(pos.sfen()));
				if (current_child_book_moves == current_book.book_body.end()) {
					// �q�ǖʂ���Ճf�[�^�x�[�X�Ɋ܂܂�Ă��Ȃ������B
					// ���̎w����͂��̂܂ܒǉ�����B
					pos.undo_move(best_move);

					// ���݂̋ǖʂɑ΂��Ďw�����o�^���邽�߁Aundo_move()���Ă���o�^����
					next_book.insert(RemovePlyIfIgnoreBookPly(pos.sfen()), current_book_move);
					continue;
				}

				// �q�ǖʂ̎w����̒��ōł��]���l�̍����w�����I��
				// ���̕]���l�����݂̋ǖʂ̎w����ɓ`������
				int best_value = -VALUE_MATE;
				Move next_move = MOVE_NONE;
				int depth = 0;
				for (const auto& current_child_book_move : *current_child_book_moves->second) {
					if (best_value < current_child_book_move.value) {
						best_value = current_child_book_move.value;
						next_move = current_child_book_move.bestMove;
					}
					depth = std::max(depth, current_child_book_move.depth);
				}

				pos.undo_move(best_move);

				// ���݂̋ǖʂɑ΂��Ďw�����o�^���邽�߁Aundo_move()���Ă���o�^����
				next_book.insert(RemovePlyIfIgnoreBookPly(pos.sfen()),
					BookPos(best_move, next_move, -best_value, depth, 1));
			}
		}

		std::swap(current_book, next_book);

		char output_book_file_for_this_depth[_MAX_PATH];
		sprintf(output_book_file_for_this_depth, "book/%s.%02d", output_book_file.c_str(), depth);
		WriteBook(current_book, output_book_file_for_this_depth);
		sync_cout << "|output_book|=" << current_book.book_body.size() << sync_endl;
	}

	return true;
}

namespace {
	// �^����ꂽ�ǖʂɂ�����A�^����ꂽ�w���肪�A
	// ��Ճf�[�^�x�[�X�̒��ň����w����Ƃ���Ă��邩���f����B
	bool IsBadMove(BookMoveSelector& book, Position& position, Move move32, int book_eval_black_limit, int book_eval_white_limit) {
		auto book_moves = book.GetMemoryBook().find(position);
		if (book_moves == nullptr) {
			// ��Ճf�[�^�x�[�X�ɁA���̋ǖʂ��o�^����Ă��Ȃ��ꍇ�B
			return false;
		}

		auto book_move = std::find_if(book_moves->begin(), book_moves->end(),
			[move32](auto& m) {
				return To16Bit(m.bestMove) == To16Bit(move32);
			});
		if (book_move == book_moves->end()) {
			// ��Ճf�[�^�x�[�X�ɁA���̎w���肪�o�^����Ă��Ȃ��ꍇ�B
			return false;
		}

		int book_eval_limit = position.side_to_move() == BLACK ? book_eval_black_limit : book_eval_white_limit;
		// �w����ɕt�^����Ă���]���l���A�w�肳�ꂽ臒l�ȉ��������ꍇ�A�����w����Ɣ��f����B
		return book_move->value < book_eval_limit;
	}

	// �^����ꂽ�ǖʂ��������ׂ����ǂ������f����
	bool IsTargetPosition(BookMoveSelector& book, Position& position, int multi_pv) {
		auto book_moves = book.GetMemoryBook().find(position);
		if (book_moves == nullptr) {
			// ��Ճf�[�^�x�[�X�ɁA���̋ǖʂ��o�^����Ă��Ȃ��ꍇ�A��������B
			return true;
		}

		// �o�^����Ă���w����̐����AMultiPV��菭�Ȃ��ꍇ�A��������B
		return book_moves->size() < multi_pv;
	}
}

// ��Ղ̉����̑ΏۂƂȂ�ǖʂ𒊏o����B
// MultiPV���w�肳�ꂽ�l���Ⴂ�ǖʁA����ђ�Ճf�[�^�x�[�X�Ɋ܂܂�Ă��Ȃ��ǖʂ��ΏۂƂȂ�B
// ���炩���߁ASetScoreToMove()��p���A��Ճf�[�^�x�[�X�̊e�w����ɕ]���l��t���A
// PropagateLeafNodeValuesToRoot()��p���ALeaf�ǖʂ̕]���l��Root�ǖʂɓ`�����Ă���g�p����B
bool Tanuki::ExtractTargetPositions() {
	int multi_pv = (int)Options[kMultiPV];
	std::string input_book_file = Options[kBookInputFile];
	int book_eval_black_limit = (int)Options["BookEvalBlackLimit"];
	int book_eval_white_limit = (int)Options["BookEvalWhiteLimit"];
	std::string target_sfens_file = Options[kBookTargetSfensFile];

	sync_cout << "info string multi_pv=" << multi_pv << sync_endl;
	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string book_eval_black_limit=" << book_eval_black_limit << sync_endl;
	sync_cout << "info string book_eval_white_limit=" << book_eval_white_limit << sync_endl;
	sync_cout << "info string target_sfens_file=" << sync_endl;

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

	std::set<std::string> explorered;
	explorered.insert(SFEN_HIRATE);
	// �����̏������̂��߁A����ǖʂ���̎w����Ƃ��ĕێ�����
	// Move��32�r�b�g�łƂ���
	std::deque<std::vector<Move>> frontier;
	frontier.push_back({});

	std::ofstream ofs(target_sfens_file);

	int counter = 0;
	StateInfo state_info[1024] = {};
	while (!frontier.empty()) {
		if (++counter % 1000 == 0) {
			sync_cout << counter << sync_endl;
		}

		auto moves = frontier.front();
		Position position;
		position.set_hirate(state_info, Threads[0]);
		// ���ǖʂ܂Ŏw�����i�߂�
		for (auto move : moves) {
			position.do_move(move, state_info[position.game_ply()]);
		}
		frontier.pop_front();

		// �����̋ǖʂ͏������Ȃ�
		auto draw_type = position.is_repetition(MAX_PLY);
		if (draw_type == REPETITION_DRAW) {
			continue;
		}

		// �l�݁A�錾�����̋ǖʂ��������Ȃ�
		if (position.is_mated() || position.DeclarationWin() != MOVE_NONE) {
			continue;
		}

		// �q�ǖʂ�W�J����
		for (const auto& move : MoveList<LEGAL_ALL>(position)) {
			if (!position.pseudo_legal(move) || !position.legal(move)) {
				// �s���Ȏ�̏ꍇ�͏������Ȃ�
				continue;
			}

			if (IsBadMove(book, position, move, book_eval_black_limit, book_eval_white_limit)) {
				// ��Ճf�[�^�x�[�X�ɓo�^����Ă���A�������w����̏ꍇ�͏������Ȃ�
				continue;
			}

			position.do_move(move, state_info[position.game_ply()]);

			// undo_move()���Ăяo���K�v������̂ŁAcontinue��break���֎~����B
			if (!explorered.count(position.sfen())) {
				explorered.insert(position.sfen());

				// ���̋ǖʂ���Ճf�[�^�x�[�X�Ɋ܂܂�Ă���ꍇ�́A�L���[�ɒǉ�����B
				if (book.GetMemoryBook().book_body.count(position.sfen())) {
					moves.push_back(move);
					frontier.push_back(moves);
					moves.pop_back();
				}

				if (IsTargetPosition(book, position, multi_pv)) {
					// �Ώۂ̋ǖʂ������o���B
					// �`����move���X�y�[�X��؂�ŕ��ׂ����̂Ƃ���B
					// ����́A����蓙��F�������邽�߁B

					for (auto m : moves) {
						ofs << m << " ";
					}
					ofs << move << std::endl;
				}
			}

			position.undo_move(move);
		}
	}

	sync_cout << "done..." << sync_endl;

	return true;
}

bool Tanuki::AddTargetPositions() {
	int num_threads = (int)Options[kThreads];
	std::string input_book_file = Options[kBookInputFile];
	int search_depth = (int)Options[kBookSearchDepth];
	int search_nodes = (int)Options[kBookSearchNodes];
	int multi_pv = (int)Options[kMultiPV];
	std::string output_book_file = Options[kBookOutputFile];
	std::string target_sfens_file = Options[kBookTargetSfensFile];

	omp_set_num_threads(num_threads);

	sync_cout << "info string num_threads=" << num_threads << sync_endl;
	sync_cout << "info string input_book_file=" << input_book_file << sync_endl;
	sync_cout << "info string search_depth=" << search_depth << sync_endl;
	sync_cout << "info string search_nodes=" << search_nodes << sync_endl;
	sync_cout << "info string multi_pv=" << multi_pv << sync_endl;
	sync_cout << "info string output_book_file=" << output_book_file << sync_endl;
	sync_cout << "info string target_sfens_file=" << sync_endl;

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

	// �Ώۂ̋ǖʂ�ǂݍ���
	sync_cout << "Reading target positions: target_sfens_file=" << target_sfens_file << sync_endl;
	std::vector<std::string> lines;
	std::ifstream ifs(target_sfens_file);
	std::string line;
	while (std::getline(ifs, line)) {
		lines.push_back(line);
	}
	int num_positions = lines.size();
	sync_cout << "done..." << sync_endl;
	sync_cout << "|lines|=" << num_positions << sync_endl;

	ProgressReport progress_report(num_positions, kShowProgressPerAtMostSec);
	time_t last_save_time_sec = std::time(nullptr);

	std::atomic<bool> need_wait = false;
	std::atomic_int global_pos_index;
	global_pos_index = 0;
	std::atomic_int global_num_processed_positions;
	global_num_processed_positions = 0;
	std::mutex output_book_mutex;

#pragma omp parallel
	{
		int thread_index = ::omp_get_thread_num();
		WinProcGroup::bindThisThread(thread_index);

		for (int position_index = global_pos_index++; position_index < num_positions;
			position_index = global_pos_index++) {
			Thread& thread = *Threads[thread_index];
			StateInfo state_info[1024] = {};
			Position& pos = thread.rootPos;
			pos.set_hirate(state_info, &thread);

			std::istringstream iss(lines[position_index]);
			std::string move_string;
			while (iss >> move_string) {
				Move move = USI::to_move(move_string);
				move = pos.move16_to_move(move);
				pos.do_move(move, state_info[pos.game_ply()]);
			}

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
					output_book.insert(pos.sfen(), book_pos);
				}
			}

			// �i���󋵂�\������
			int num_processed_positions = ++global_num_processed_positions;
			progress_report.Show(num_processed_positions);

			// ��莞�Ԃ��Ƃɕۑ�����
			{
				std::lock_guard<std::mutex> lock(output_book_mutex);
				if (last_save_time_sec + kSavePerAtMostSec < std::time(nullptr)) {
					WriteBook(output_book, output_book_file);
					last_save_time_sec = std::time(nullptr);
				}
			}

			need_wait = need_wait ||
				(progress_report.HasDataPerTime() &&
					progress_report.GetDataPerTime() * 2 < progress_report.GetMaxDataPerTime());

			if (need_wait) {
				// �������x���ቺ���Ă��Ă���B
				// �S�ẴX���b�h��ҋ@����B
#pragma omp barrier

				// �}�X�^�[�X���b�h�ł��΂炭�ҋ@����B
#pragma omp master
				{
					sync_cout << "Speed is down. Waiting for a while. GetDataPerTime()=" <<
						progress_report.GetDataPerTime() << " GetMaxDataPerTime()=" <<
						progress_report.GetMaxDataPerTime() << sync_endl;

					std::this_thread::sleep_for(std::chrono::minutes(10));
					progress_report.Reset();
					need_wait = false;
				}

				// �}�X�^�[�X���b�h�̑ҋ@���I���܂ŁA�ēx�S�ẴX���b�h��ҋ@����B
#pragma omp barrier
			}
		}

		WriteBook(output_book, output_book_file);

		return true;
	}

}
#endif
