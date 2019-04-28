#include "tanuki_kifu_generator.h"
#include "extra/config.h"

#ifdef EVAL_LEARN

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

#include "misc.h"
#include "search.h"
#include "tanuki_kifu_writer.h"
#include "tanuki_progress_report.h"
#include "thread.h"

#ifdef abs
#undef abs
#endif

using Search::RootMove;
using USI::Option;
using USI::OptionsMap;

namespace Learner {
	std::pair<Value, std::vector<Move> > search(Position& pos, int depth, size_t multiPV);
	std::pair<Value, std::vector<Move> > qsearch(Position& pos);
}

namespace {
	constexpr int kMaxGamePlay = 400;
	constexpr int kMaxSwapTrials = 10;
	constexpr int kMaxTrialsToSelectSquares = 100;

	enum GameResult {
		GameResultWin = 1,
		GameResultLose = -1,
		GameResultDraw = 0,
	};

	constexpr char* kOptionGeneratorNumPositions = "GeneratorNumPositions";
	constexpr char* kOptionGeneratorSearchDepth = "GeneratorSearchDepth";
	constexpr char* kOptionGeneratorKifuTag = "GeneratorKifuTag";
	constexpr char* kOptionGeneratorStartposFileName = "GeneratorStartposFileName";
	constexpr char* kOptionGeneratorValueThreshold = "GeneratorValueThreshold";
	constexpr char* kOptionGeneratorOptimumNodesSearched = "GeneratorOptimumNodesSearched";
	constexpr char* kOptionGeneratorMeasureDepth = "GeneratorMeasureDepth";
	constexpr char* kOptionConvertSfenToLearningDataInputSfenFileName =
		"ConvertSfenToLearningDataInputSfenFileName";
	constexpr char* kOptionConvertSfenToLearningDataSearchDepth =
		"ConvertSfenToLearningDataSearchDepth";
	constexpr char* kOptionConvertSfenToLearningDataOutputFileName =
		"ConvertSfenToLearningDataOutputFileName";

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
			StateInfo state_infos[4096] = {};
			StateInfo* state = state_infos + 8;
			Position& pos = thread.rootPos;
			pos.set_hirate(state, &thread);

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

void Tanuki::InitializeGenerator(USI::OptionsMap& o) {
	o[kOptionGeneratorNumPositions] << Option("10000000000");
	o[kOptionGeneratorSearchDepth] << Option(8, 1, MAX_PLY);
	o[kOptionGeneratorKifuTag] << Option("default_tag");
	o[kOptionGeneratorStartposFileName] << Option("startpos.sfen");
	o[kOptionGeneratorValueThreshold] << Option(VALUE_MATE, 0, VALUE_MATE);
	o[kOptionGeneratorOptimumNodesSearched] << Option("0");
	o[kOptionGeneratorMeasureDepth] << Option(false);
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

		// 50%�̊m���ŋʂ̈ړ��݂̂��s��
		if (mt() & 1) {
			root_moves.erase(std::remove_if(root_moves.begin(), root_moves.end(),
				[&pos](const auto& root_move) {
				Move move = root_move.pv.front();
				if (is_drop(move)) {
					return true;
				}
				Square sq = from_sq(move);
				return pos.king_square(BLACK) != sq &&
					pos.king_square(WHITE) != sq;
			}),
				root_moves.end());
		}

		if (root_moves.empty()) {
			return;
		}

		std::uniform_int_distribution<> dist(0, static_cast<int>(root_moves.size()) - 1);
		pos.do_move(root_moves[dist(mt)].pv[0], state[pos.game_ply()]);
		Eval::evaluate(pos);
	}
}

void Tanuki::GenerateKifu() {
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	// ��Ղ̓ǂݍ���
	if (!ReadBook()) {
		sync_cout << "Failed to read the book." << sync_endl;
		return;
	}

	omp_set_num_threads((int)Options["Threads"]);

	Eval::load_eval();

	std::string kifu_directory = (std::string)Options["KifuDir"];
	_mkdir(kifu_directory.c_str());

	int search_depth = Options[kOptionGeneratorSearchDepth];
	int64_t num_positions = ParseOptionOrDie<int64_t>(kOptionGeneratorNumPositions);
	int value_threshold = Options[kOptionGeneratorValueThreshold];
	std::string output_file_name_tag = Options[kOptionGeneratorKifuTag];
	uint64_t optimum_nodes_searched =
		ParseOptionOrDie<uint64_t>(kOptionGeneratorOptimumNodesSearched);
	bool measure_depth = Options[kOptionGeneratorMeasureDepth];

	std::cout << "search_depth=" << search_depth << std::endl;
	std::cout << "num_positions=" << num_positions << std::endl;
	std::cout << "value_threshold=" << value_threshold << std::endl;
	std::cout << "output_file_name_tag=" << output_file_name_tag << std::endl;
	std::cout << "optimum_nodes_searched=" << optimum_nodes_searched << std::endl;
	std::cout << "measure_depth=" << measure_depth << std::endl;

	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
	limits.max_game_ply = 1 << 16;
	limits.depth = MAX_PLY;
	limits.silent = true;
	limits.enteringKingRule = EKR_27_POINT;
	limits.nodes_per_thread = optimum_nodes_searched;
	Search::Limits = limits;

	// ����� -> �T���[��
	std::vector<std::vector<int> > game_play_to_depths(kMaxGamePlay + 1);

	time_t start_time;
	std::time(&start_time);
	ASSERT_LV3(start_positions.size());
	std::uniform_int_distribution<> start_positions_index(0, static_cast<int>(start_positions.size() - 1));
	// �X���b�h�Ԃŋ��L����
	std::atomic_int64_t global_position_index;
	global_position_index = 0;
	ProgressReport progress_report(num_positions, 10 * 60);
#pragma omp parallel
	{
		int thread_index = ::omp_get_thread_num();
		WinProcGroup::bindThisThread(thread_index);
		char output_file_path[1024];
		std::sprintf(output_file_path, "%s/kifu.%s.%d.%I64d.%03d.%I64d.bin", kifu_directory.c_str(),
			output_file_name_tag.c_str(), search_depth, num_positions, thread_index,
			start_time);
		// �e�X���b�h�Ɏ�������
		std::unique_ptr<KifuWriter> kifu_writer =
			std::make_unique<KifuWriter>(output_file_path);
		std::mt19937_64 mt19937_64(start_time + thread_index);

		while (global_position_index < num_positions) {
			Thread& thread = *Threads[thread_index];
			StateInfo state_infos[4096] = {};
			StateInfo* state = state_infos + 8;
			Position& pos = thread.rootPos;
			pos.set(start_positions[start_positions_index(mt19937_64)], state, &thread);

			RandomMove(pos, state, mt19937_64);

			std::vector<Learner::PackedSfenValue> records;
			Value last_value;
			while (pos.game_ply() < kMaxGamePlay && !pos.is_mated() &&
				pos.DeclarationWin() == MOVE_NONE) {
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

				Move pv_move = pv[0];

				Learner::PackedSfenValue record = {};
				pos.sfen_pack(record.sfen);
				record.score = last_value;
				record.gamePly = pos.game_ply();
				record.move = pv_move;
				records.push_back(record);

				pos.do_move(pv_move, state[pos.game_ply()]);
				// �����v�Z�̂���evaluate()���Ăяo��
				Eval::evaluate(pos);

				// �萔���̒T���̐[�����L�^���Ă���
				// ���炩�̌`�ŏ��������܂��Ă���ǖʂ�
				// �T���[�����ɒ[�ɐ[���Ȃ邽�ߏ��O����
				if (measure_depth && abs(last_value) < VALUE_KNOWN_WIN) {
#pragma omp critical
					{
						game_play_to_depths[pos.game_ply()].push_back(thread.rootDepth);
					}
				}
			}

			int game_result = GameResultDraw;
			Color win;
			RepetitionState repetition_state = pos.is_repetition(0);
			if (pos.is_mated()) {
				// ����
				// �l�܂��ꂽ
				// �Ō�̋ǖʂ͑���ǖʂȂ̂ŏ���
				game_result = GameResultWin;
			}
			else if (pos.DeclarationWin() != MOVE_NONE) {
				// ����
				// ���ʏ���
				// �Ō�̋ǖʂ͑���ǖʂȂ̂ŕ���
				game_result = GameResultLose;
			}
			else if (last_value > value_threshold) {
				// ����
				// �Ō�̋ǖʂ͑���ǖʂȂ̂ŕ���
				game_result = GameResultLose;
			}
			else if (last_value < -value_threshold) {
				// ����
				// �Ō�̋ǖʂ͑���ǖʂȂ̂ŏ���
				game_result = GameResultWin;
			}
			else {
				continue;
			}

			for (auto rit = records.rbegin(); rit != records.rend(); ++rit) {
				rit->game_result = game_result;
				game_result = -game_result;
			}

			if (!records.empty()) {
				records.back().last_position = true;
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
		Threads.stop = true;
	}

	if (measure_depth) {
		char output_file_path[1024];
		std::sprintf(output_file_path, "%s/kifu.%s.%d.%I64d.%I64d.search_depth.csv", kifu_directory.c_str(),
			output_file_name_tag.c_str(), search_depth, num_positions, start_time);

		FILE* file = std::fopen(output_file_path, "wt");
		fprintf(file, "game_play,N,average,sd\n");

		for (int game_play = 0; game_play <= kMaxGamePlay; ++game_play) {
			double sum = 0.0;
			double sum2 = 0.0;
			int N = static_cast<int>(game_play_to_depths[game_play].size());
			for (auto search_depth : game_play_to_depths[game_play]) {
				sum += search_depth;
				sum2 += search_depth * search_depth;
			}

			if (N) {
				double average = sum / N;
				double sd = sum2 / N - average * average;
				fprintf(file, "%d,%d,%f,%f\n", game_play, N, average, sd);
			}
			else {
				fprintf(file, "%d,%d,%f,%f\n", game_play, N, 0.0, 0.0);
			}
		}
	}
}

void Tanuki::ConvertSfenToLearningData() {
	Eval::load_eval();

	omp_set_num_threads((int)Options["Threads"]);

	Search::LimitsType limits;
	// ���������̎萔�t�߂ň��������̒l���Ԃ�̂�h������1 << 16�ɂ���
	limits.max_game_ply = 1 << 16;
	limits.depth = MAX_PLY;
	limits.silent = true;
	limits.enteringKingRule = EKR_27_POINT;
	Search::Limits = limits;

	std::string input_sfen_file_name =
		(std::string)Options[kOptionConvertSfenToLearningDataInputSfenFileName];
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
	std::atomic_int64_t global_sfen_index;
	global_sfen_index = 0;
	int64_t num_sfens = sfens.size();
	ProgressReport progress_report(num_sfens, 60);
	std::unique_ptr<KifuWriter> kifu_writer =
		std::make_unique<KifuWriter>(output_file_name);
	std::mutex mutex;
#pragma omp parallel
	{
		int thread_index = ::omp_get_thread_num();
		WinProcGroup::bindThisThread(thread_index);

		for (int64_t sfen_index = global_sfen_index++; sfen_index < num_sfens;
			sfen_index = global_sfen_index++) {
			const std::string& sfen = sfens[sfen_index];
			Thread& thread = *Threads[thread_index];
			StateInfo state_infos[4096] = {};
			StateInfo* state = state_infos + 8;
			Position& pos = thread.rootPos;
			pos.set_hirate(state, &thread);

			std::istringstream iss(sfen);
			// startpos moves 7g7f 3c3d 2g2f
			std::vector<Learner::PackedSfenValue> records;
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

				Learner::PackedSfenValue record = {};
				pos.sfen_pack(record.sfen);
				record.score = root_move.score;
				record.gamePly = pos.game_ply();
				records.push_back(record);

				if (pos.DeclarationWin()) {
					win = pos.side_to_move();
					break;
				}
			}

			// sync_cout << pos << sync_endl;
			// pos.DeclarationWin();

			if (win == COLOR_NB) {
				sync_cout << "Skipped..." << sync_endl;
				continue;
			}
			sync_cout << "DeclarationWin..." << sync_endl;

			int game_result = GameResultWin;
			for (int i = static_cast<int>(records.size()) - 1; i >= 0; --i) {
				records[i].game_result = game_result;
				game_result = -game_result;
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

#endif