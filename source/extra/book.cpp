﻿#include "../shogi.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include "book.h"
#include "../position.h"
#include "../misc.h"
#include "../search.h"
#include "../thread.h"
#include "../learn/multi_think.h"

using namespace std;
using std::cout;

void is_ready();

namespace Book
{
#ifdef ENABLE_MAKEBOOK_CMD
	// ----------------------------------
	// USI拡張コマンド "makebook"(定跡作成)
	// ----------------------------------

	// 局面を与えて、その局面で思考させるために、やねうら王2016Midが必要。
#if defined(EVAL_LEARN) && (defined(YANEURAOU_2016_MID_ENGINE) || defined(YANEURAOU_2016_LATE_ENGINE))
	struct MultiThinkBook: public MultiThink
	{
		MultiThinkBook(int search_depth_, MemoryBook & book_)
			: search_depth(search_depth_), book(book_), appended(false) {}

		virtual void thread_worker(size_t thread_id);

		// 定跡を作るために思考する局面
		vector<string> sfens;

		// 定跡を作るときの通常探索の探索深さ
		int search_depth;

		// メモリ上の定跡ファイル(ここに追加していく)
		MemoryBook& book;

		// 前回から新たな指し手が追加されたかどうかのフラグ。
		bool appended;
	};


	//  thread_id    = 0..Threads.size()-1
	//  search_depth = 通常探索の探索深さ
	void MultiThinkBook::thread_worker(size_t thread_id)
	{
		// g_loop_maxになるまで繰り返し
		u64 id;
		while ((id = get_next_loop_count()) != UINT64_MAX)
		{
			auto sfen = sfens[id];

			auto& pos = Threads[thread_id]->rootPos;
			pos.set(sfen);
			auto th = Threads[thread_id];
			pos.set_this_thread(th);

			if (pos.is_mated())
				continue;

			// depth手読みの評価値とPV(最善応手列)
			search(pos, -VALUE_INFINITE, VALUE_INFINITE, search_depth);

			// MultiPVで局面を足す、的な

			vector<BookPos> move_list;

			int multi_pv = std::min((int)Options["MultiPV"], (int)th->rootMoves.size());
			for (int i = 0; i < multi_pv; ++i)
			{
				// 出現頻度は、バージョンナンバーを100倍したものにしておく)
				Move nextMove = (th->rootMoves[i].pv.size() >= 1) ? th->rootMoves[i].pv[1] : MOVE_NONE;
				BookPos bp(th->rootMoves[i].pv[0], nextMove, th->rootMoves[i].score
					, search_depth, int(atof(ENGINE_VERSION) * 100));

				// MultiPVで思考しているので、手番側から見て評価値の良い順に並んでいることは保証される。
				// (書き出しのときに並び替えなければ)
				move_list.push_back(bp);
			}

			{
				std::unique_lock<Mutex> lk(io_mutex);
				// 前のエントリーは上書きされる。
				book.book_body[sfen] = move_list;

				// 新たなエントリーを追加したのでフラグを立てておく。
				appended = true;
			}

			// 1局面思考するごとに'.'をひとつ出力する。
			cout << '.';
		}
	}
#endif

	// フォーマット等についてはdoc/解説.txt を見ること。
	void makebook_cmd(Position& pos, istringstream& is)
	{
		string token;
		is >> token;

		// sfenから生成する
		bool from_sfen = token == "from_sfen";
		// 自ら思考して生成する
		bool from_thinking = token == "think";
		// 定跡のマージ
		bool book_merge = token == "merge";
		// 定跡のsort
		bool book_sort = token == "sort";

#if !defined(EVAL_LEARN) || !(defined(YANEURAOU_2016_MID_ENGINE) || defined(YANEURAOU_2016_LATE_ENGINE))
		if (from_thinking)
		{
			cout << "Error!:define EVAL_LEARN and YANEURAOU_2016_MID_ENGINE" << endl;
			return;
		}
#endif

		if (from_sfen || from_thinking)
		{
			// sfenファイル名
			is >> token;
			string sfen_name = token;

			// 定跡ファイル名
			string book_name;
			is >> book_name;

			// 開始手数、終了手数、探索深さ
			int start_moves = 1;
			int moves = 16;
			int depth = 24;
			while (true)
			{
				token = "";
				is >> token;
				if (token == "")
					break;
				if (token == "moves")
					is >> moves;
				else if (token == "depth")
					is >> depth;
				else if (from_thinking && token == "startmoves")
					is >> start_moves;
				else
				{
					cout << "Error! : Illigal token = " << token << endl;
					return;
				}
			}

			if (from_sfen)
				cout << "read sfen moves " << moves << endl;
			if (from_thinking)
				cout << "read sfen moves from " << start_moves << " to " << moves << " , depth = " << depth << endl;

			vector<string> sfens;
			read_all_lines(sfen_name, sfens);

			cout << "..done" << endl;

			MemoryBook book;

			if (from_thinking)
			{
				cout << "read book..";
				// 初回はファイルがないので読み込みに失敗するが無視して続行。
				if (read_book(book_name, book) != 0)
				{
					cout << "..but , create new file." << endl;
				} else
					cout << "..done" << endl;
			}

			// この時点で評価関数を読み込まないとKPPTはPositionのset()が出来ないので…。
			is_ready();

			cout << "parse..";

			// 思考すべき局面のsfen
			unordered_set<string> thinking_sfens;

			// 各行の局面をparseして読み込む(このときに重複除去も行なう)
			for (size_t k = 0; k < sfens.size(); ++k)
			{
				auto sfen = sfens[k];

				if (sfen.length() == 0)
					continue;

				istringstream iss(sfen);
				token = "";
				do {
					iss >> token;
				} while (token == "startpos" || token == "moves");

				vector<Move> m;    // 初手から(moves+1)手までの指し手格納用
				vector<string> sf; // 初手から(moves+0)手までのsfen文字列格納用

				StateInfo si[MAX_PLY];

				pos.set_hirate();

				// sfenから直接生成するときはponderのためにmoves + 1の局面まで調べる必要がある。
				for (int i = 0; i < moves + (from_sfen ? 1 : 0); ++i)
				{
					// 初回は、↑でfeedしたtokenが入っているはず。
					if (i != 0)
					{
						token = "";
						iss >> token;
					}
					if (token == "")
						break;

					Move move = move_from_usi(pos, token);
					// illigal moveであるとMOVE_NONEが返る。
					if (move == MOVE_NONE)
					{
						cout << "illegal move : line = " << (k + 1) << " , " << sfen << " , move = " << token << endl;
						break;
					}

					// MOVE_WIN,MOVE_RESIGNでは局面を進められないのでここで終了。
					if (!is_ok(move))
						break;

					sf.push_back(pos.sfen());
					m.push_back(move);

					pos.do_move(move, si[i]);
				}

				for (int i = 0; i < (int)m.size() - (from_sfen ? 1 : 0); ++i)
				{
					if (i < start_moves - 1)
						continue;

					if (from_sfen)
					{
						// この場合、m[i + 1]が必要になるので、m.size()-1までしかループできない。
						BookPos bp(m[i], m[i + 1], VALUE_ZERO, 32, 1);
						insert_book_pos(book, sf[i], bp);
					} else if (from_thinking)
					{
						// posの局面で思考させてみる。(あとでまとめて)
						if (thinking_sfens.count(sf[i]) == 0)
							thinking_sfens.insert(sf[i]);
					}
				}

				// sfenから生成するモードの場合、1000棋譜処理するごとにドットを出力。
				if ((k % 1000) == 0)
					cout << '.';
			}
			cout << "done." << endl;

#if defined(EVAL_LEARN) && (defined(YANEURAOU_2016_MID_ENGINE)||defined(YANEURAOU_2016_ENGINE))

			if (from_thinking)
			{
				// thinking_sfensを並列的に探索して思考する。
				// スレッド数(これは、USIのsetoptionで与えられる)
				u32 multi_pv = Options["MultiPV"];

				// 思考する局面をsfensに突っ込んで、この局面数をg_loop_maxに代入しておき、この回数だけ思考する。
				MultiThinkBook multi_think(depth, book);

				auto& sfens_ = multi_think.sfens;
				for (auto& s : thinking_sfens)
				{

					// この局面のいま格納されているデータを比較して、この局面を再考すべきか判断する。
					auto it = book.book_body.find(s);

					// MemoryBookにエントリーが存在しないなら無条件で、この局面について思考して良い。
					if (it == book.book_body.end())
						sfens_.push_back(s);
					else
					{
						auto& bp = it->second;
						if (bp[0].depth < depth // 今回の探索depthのほうが深い
							|| (bp[0].depth == depth && bp.size() < multi_pv) // 探索深さは同じだが今回のMultiPVのほうが大きい
							)
							sfens_.push_back(s);
					}
				}

#if 0
				// 思考対象局面が求まったので、sfenを表示させてみる。
				cout << "thinking sfen = " << endl;
				for (auto& s : sfens_)
					cout << "sfen " << s << endl;
#endif
				// 思考対象node数の出力。
				cout << "total " << sfens_.size() << " nodes " << endl;

				multi_think.set_loop_max(sfens_.size());

				// 30分ごとに保存
				// (ファイルが大きくなってくると保存の時間も馬鹿にならないのでこれくらいの間隔で妥協)
				multi_think.callback_seconds = 30 * 60;
				multi_think.callback_func = [&]()
				{
					std::unique_lock<Mutex> lk(multi_think.io_mutex);
					// 前回書き出し時からレコードが追加された？
					if (multi_think.appended)
					{
						write_book(book_name, book);
						cout << 'S';
						multi_think.appended = false;
					} else {
						// 追加されていないときは小文字のsマークを表示して
						// ファイルへの書き出しは行わないように変更。
						cout << 's';
					}
				};

				multi_think.go_think();

			}

#endif

			cout << "write..";
			write_book(book_name, book);
			cout << "finished." << endl;

		} else if (book_merge) {

			// 定跡のマージ
			MemoryBook book[3];
			string book_name[3];
			is >> book_name[0] >> book_name[1] >> book_name[2];
			if (book_name[2] == "")
			{
				cout << "Error! book name is empty." << endl;
				return;
			}
			cout << "book merge from " << book_name[0] << " and " << book_name[1] << " to " << book_name[2] << endl;
			for (int i = 0; i < 2; ++i)
			{
				if (read_book(book_name[i], book[i]) != 0)
					return;
			}

			// 読み込めたので合体させる。
			cout << "merge..";

			// 同一nodeと非同一nodeの統計用
			u64 same_nodes = 0, diffrent_nodes = 0;

			// 1) 探索が深いほうを採用。
			// 2) 同じ探索深さであれば、MultiPVの大きいほうを採用。
			for (auto& it0 : book[0].book_body)
			{
				auto sfen = it0.first;
				// このエントリーがbook1のほうにないかを調べる。
				auto it1_ = book[1].book_body.find(sfen);
				auto& it1 = *it1_;
				if (it1_ != book[1].book_body.end())
				{
					same_nodes++;

					// あったので、良いほうをbook2に突っ込む。
					// 1) 登録されている候補手の数がゼロならこれは無効なのでもう片方を登録
					// 2) depthが深いほう
					// 3) depthが同じならmulti pvが大きいほう(登録されている候補手が多いほう)
					if (it0.second.size() == 0)
						book[2].book_body.insert(it1);
					else if (it1.second.size() == 0)
						book[2].book_body.insert(it0);
					else if (it0.second[0].depth > it1.second[0].depth)
						book[2].book_body.insert(it0);
					else if (it0.second[0].depth < it1.second[0].depth)
						book[2].book_body.insert(it1);
					else if (it0.second.size() >= it1.second.size())
						book[2].book_body.insert(it0);
					else
						book[2].book_body.insert(it1);
				} else {
					// なかったので無条件でbook2に突っ込む。
					book[2].book_body.insert(it0);

					diffrent_nodes++;
				}
			}
			// book0の精査が終わったので、book1側で、まだ突っ込んでいないnodeを探して、それをbook2に突っ込む
			for (auto& it1 : book[1].book_body)
			{
				if (book[2].book_body.find(it1.first) == book[2].book_body.end())
					book[2].book_body.insert(it1);
			}
			cout << "..done" << endl;
			cout << "same nodes = " << same_nodes << " , different nodes =  " << diffrent_nodes << endl;

			cout << "write..";
			write_book(book_name[2], book[2]);
			cout << "..done!" << endl;

		} else if (book_sort) {
			// 定跡のsort
			MemoryBook book;
			string book_src, book_dst;
			is >> book_src >> book_dst;
			cout << "book sort from " << book_src << " , write to " << book_dst << endl;
			Book::read_book(book_src, book);

			cout << "write..";
			write_book(book_dst, book, true);
			cout << "..done!" << endl;

		} else {
			cout << "usage" << endl;
			cout << "> makebook from_sfen book.sfen book.db moves 24" << endl;
			cout << "> makebook think book.sfen book.db moves 16 depth 18" << endl;
			cout << "> makebook merge book_src1.db book_src2.db book_merged.db" << endl;
			cout << "> makebook sort book_src.db book_sorted.db" << endl;
		}
	}
#endif

	// 定跡ファイルの読み込み(book.db)など。
	int read_book(const std::string& filename, MemoryBook& book, bool on_the_fly)
	{
		// 読み込み済であるかの判定
		if (book.book_name == filename)
			return 0;

		// 別のファイルを開こうとしているなら前回メモリに丸読みした定跡をクリアしておかないといけない。
		if (book.book_name != "")
			book.book_body.clear();

		// ファイルだけオープンして読み込んだことにする。
		if (on_the_fly)
		{
			if (book.fs.is_open())
				book.fs.close();

			book.fs.open(filename, ios::in);
			if (book.fs.fail())
			{
				cout << "info string Error! : can't read " + filename << endl;
				return 1;
			}

			book.on_the_fly = true;
			book.book_name = filename;
			return 0;
		}

		vector<string> lines;
		if (read_all_lines(filename, lines))
		{
			cout << "info string Error! : can't read " + filename << endl;
			//      exit(EXIT_FAILURE);
			return 1; // 読み込み失敗
		}

		uint64_t num_sum = 0;
		string sfen;

		auto calc_prob = [&] {
			auto& move_list = book.book_body[sfen];
			std::stable_sort(move_list.begin(), move_list.end());
			num_sum = std::max(num_sum, UINT64_C(1)); // ゼロ除算対策
			for (auto& bp : move_list)
				bp.prob = float(bp.num) / num_sum;
			num_sum = 0;
		};

		for (auto line : lines)
		{
			// バージョン識別文字列(とりあえず読み飛ばす)
			if (line.length() >= 1 && line[0] == '#')
				continue;

			// コメント行(とりあえず読み飛ばす)
			if (line.length() >= 2 && line.substr(0, 2) == "//")
				continue;

			// "sfen "で始まる行は局面のデータであり、sfen文字列が格納されている。
			if (line.length() >= 5 && line.substr(0, 5) == "sfen ")
			{
				// ひとつ前のsfen文字列に対応するものが終わったということなので採択確率を計算して、かつ、採択回数でsortしておく
				// (sortはされてるはずだが他のソフトで生成した定跡DBではそうとも限らないので)。
				calc_prob();

				sfen = line.substr(5, line.length() - 5); // 新しいsfen文字列を"sfen "を除去して格納
				continue;
			}

			Move best, next;
			int value;
			int depth;

			istringstream is(line);
			string bestMove, nextMove;
			uint64_t num;
			is >> bestMove >> nextMove >> value >> depth >> num;

#if 0
			// 思考した指し手に対しては指し手の出現頻度のところを強制的にエンジンバージョンを100倍したものに変更する。
			// この#ifを有効にして、makebook mergeコマンドを叩いて、別のファイルに書き出すなどするときに便利。
			num = int(atof(ENGINE_VERSION) * 100);
#endif

			// 起動時なので変換に要するオーバーヘッドは最小化したいので合法かのチェックはしない。
			if (bestMove == "none" || bestMove == "resign")
				best = MOVE_NONE;
			else
				best = move_from_usi(bestMove);

			if (nextMove == "none" || nextMove == "resign")
				next = MOVE_NONE;
			else
				next = move_from_usi(nextMove);

			BookPos bp(best, next, value, depth, num);
			insert_book_pos(book, sfen, bp);
			num_sum += num;
		}
		// ファイルが終わるときにも最後の局面に対するcalc_probが必要。
		calc_prob();

		// 読み込んだファイル名を保存しておく。二度目のread_book()はskipする。
		book.book_name = filename;

		return 0;
	}

	// 定跡ファイルの書き出し
	int write_book(const std::string& filename, const MemoryBook& book, bool sort)
	{
		fstream fs;
		fs.open(filename, ios::out);

		// バージョン識別用文字列
		fs << "#YANEURAOU-DB2016 1.00" << endl;

		vector<pair<string, vector<BookPos>> > vectored_book;
		for (auto& it : book.book_body)
		{
			// 指し手のない空っぽのentryは書き出さないように。
			if (it.second.size() == 0)
				continue;
			vectored_book.push_back(it);
		}

		if (sort)
		{
			// sfen文字列は手駒の表記に揺れがある。
			// (USI原案のほうでは規定されているのだが、将棋所が採用しているUSIプロトコルではこの規定がない。)
			// sortするタイミングで、一度すべての局面を読み込み、sfen()化しなおすことで
			// やねうら王が用いているsfenの手駒表記(USI原案)に統一されるようにする。

			{
				// Position::set()で評価関数の読み込みが必要。
				is_ready();
				Position pos;

				// std::vectorにしてあるのでit.firstを書き換えてもitは無効にならないはず。
				for (auto& it : vectored_book)
				{
					pos.set(it.first);
					it.first = pos.sfen();
				}
			}


			// ここvectored_bookが、sfen文字列でsortされていて欲しいのでsortする。
			// アルファベットの範囲ではlocaleの影響は受けない…はず…。
			std::sort(vectored_book.begin(), vectored_book.end(),
				[](const pair<string, vector<BookPos>>&lhs, const pair<string, vector<BookPos>>&rhs) {
				return lhs.first < rhs.first;
			});
		}

		for (auto& it : vectored_book)
		{
			fs << "sfen " << it.first /* is sfen string */ << endl; // sfen

			auto& move_list = it.second;

			// 採択回数でソートしておく。
			std::stable_sort(move_list.begin(), move_list.end());

			for (auto& bp : move_list)
				fs << bp.bestMove << ' ' << bp.nextMove << ' ' << bp.value << " " << bp.depth << " " << bp.num << endl;
			// 指し手、相手の応手、そのときの評価値、探索深さ、採択回数
		}

		fs.close();

		return 0;
	}

	void insert_book_pos(MemoryBook& book, const std::string sfen, const BookPos& bp)
	{
		auto it = book.book_body.find(sfen);
		if (it == book.end())
		{
			// 存在しないので要素を作って追加。
			vector<BookPos> move_list;
			move_list.push_back(bp);
			book.book_body[sfen] = move_list;
		} else {
			// この局面での指し手のリスト
			auto& move_list = it->second;
			// すでに格納されているかも知れないので同じ指し手がないかをチェックして、なければ追加
			for (auto& b : move_list)
				if (b == bp)
				{
					// すでに存在していたのでエントリーを置換。ただし採択回数はインクリメント
					auto num = b.num;
					b = bp;
					b.num += num;
					goto FOUND_THE_SAME_MOVE;
				}

			move_list.push_back(bp);

		FOUND_THE_SAME_MOVE:;
		}

	}

	// sfen文字列から末尾のゴミを取り除いて返す。
	// ios::binaryでopenした場合などには'\r'なども入っていると思われる。
	string trim_sfen(string sfen)
	{
		string s = sfen;
		int cur = (int)s.length() - 1;
		while (cur >= 0)
		{
			char c = s[cur];
			// 改行文字、スペース、数字(これはgame ply)ではないならループを抜ける。
			// これらの文字が出現しなくなるまで末尾を切り詰める。
			if (c != '\r' && c != '\n' && c != ' ' && !('0' <= c && c <= '9'))
				break;
			cur--;
		}
		s.resize((int)(cur + 1));
		return s;
	}

	MemoryBook::BookType::iterator MemoryBook::find(const Position& pos)
	{
		auto sfen = pos.sfen();
		BookType::iterator it;

		if (on_the_fly)
		{
			// ディスクから読み込むなら、いずれにせよ、book_bodyをクリアして、
			// ディスクから読み込んだエントリーをinsertしてそのiteratorを返すべき。
			book_body.clear();

			// 末尾の手数は取り除いておく。
			// read_book()で取り除くと、そのあと書き出すときに手数が消失するのでまずい。(気がする)
			sfen = trim_sfen(sfen);

			// ファイル自体はオープンされてして、ファイルハンドルはfsだと仮定して良い。

			// ファイルサイズ取得
			// C++的には未定義動作だが、これのためにsys/stat.hをincludeしたくない。
			// ここでfs.clear()を呼ばないとeof()のあと、tellg()が失敗する。
			fs.clear();
			fs.seekg(0, std::ios::end);
			auto file_end = fs.tellg();

			fs.clear();
			fs.seekg(0, std::ios::beg);
			auto file_start = fs.tellg();

			auto file_size = u64(file_end - file_start);

			// 与えられたseek位置から"sfen"文字列を探し、それを返す。どこまでもなければ""が返る。
			// hackとして、seek位置は-80しておく。
			auto next_sfen = [&](u64 seek_from)
			{
				string line;

				fs.seekg(max(s64(0), (s64)seek_from - 80), fstream::beg);
				getline(fs, line); // 1行読み捨てる

				// getlineはeof()を正しく反映させないのでgetline()の返し値を用いる必要がある。
				while (getline(fs, line))
				{
					if (!line.compare(0, 4, "sfen"))
						return trim_sfen(line.substr(5));
					// "sfen"という文字列は取り除いたものを返す。
					// 手数の表記も取り除いて比較したほうがいい。
					// ios::binaryつけているので末尾に'\R'が付与されている。禿げそう。
				}
				return string();
			};

			// バイナリサーチ

			u64 s = 0, e = file_size, m;

			while (true)
			{
				m = (s + e) / 2;

				auto sfen2 = next_sfen(m);
				if (sfen2 == "" || sfen < sfen2)
				{ // 左(それより小さいところ)を探す
					e = m;
				} else if (sfen > sfen2)
				{ // 右(それより大きいところ)を探す
					s = u64(fs.tellg() - file_start);
				} else {
					// 見つかった！
					break;
				}

				// 40バイトより小さなsfenはありえないので探索範囲がこれより小さいなら終了。
				// ただしs = 0のままだと先頭が探索されていないので..
				// s,eは無符号型であることに注意。if (s-40 < e) と書くとs-40がマイナスになりかねない。
				if (s + 40 > e)
				{
					if (s != 0 || e != 0)
					{
						// 見つからなかった
						return book_body.end();
					}

					// もしかしたら先頭付近にあるかも知れん..
					e = 0; // この条件で再度サーチ
				}

			}
			// 見つけた処理

			// read_bookとほとんど同じ読み込み処理がここに必要。辛い。

			uint64_t num_sum = 0;

			auto calc_prob = [&] {
				auto& move_list = book_body[sfen];
				std::stable_sort(move_list.begin(), move_list.end());
				num_sum = std::max(num_sum, UINT64_C(1)); // ゼロ除算対策
				for (auto& bp : move_list)
					bp.prob = float(bp.num) / num_sum;
				num_sum = 0;
			};

			while (!fs.eof())
			{
				string line;
				getline(fs, line);

				// バージョン識別文字列(とりあえず読み飛ばす)
				if (line.length() >= 1 && line[0] == '#')
					continue;

				// コメント行(とりあえず読み飛ばす)
				if (line.length() >= 2 && line.substr(0, 2) == "//")
					continue;

				// 次のsfenに遭遇したらこれにて終了。
				if (line.length() >= 5 && line.substr(0, 5) == "sfen ")
				{
					break;
				}

				Move best, next;
				int value;
				int depth;

				istringstream is(line);
				string bestMove, nextMove;
				uint64_t num;
				is >> bestMove >> nextMove >> value >> depth >> num;

				// 起動時なので変換に要するオーバーヘッドは最小化したいので合法かのチェックはしない。
				if (bestMove == "none" || bestMove == "resign")
					best = MOVE_NONE;
				else
					best = move_from_usi(bestMove);

				if (nextMove == "none" || nextMove == "resign")
					next = MOVE_NONE;
				else
					next = move_from_usi(nextMove);

				BookPos bp(best, next, value, depth, num);
				insert_book_pos(*this, sfen, bp);
				num_sum += num;
			}
			// ファイルが終わるときにも最後の局面に対するcalc_probが必要。
			calc_prob();

			it = book_body.begin();

		} else {

			// on the flyではない場合
			it = book_body.find(sfen);
		}


		if (it != book_body.end())
		{
			// 定跡のMoveは16bitであり、rootMovesは32bitのMoveであるからこのタイミングで補正する。
			for (auto& m : it->second)
				m.bestMove = pos.move16_to_move(m.bestMove);
		}
		return it;
	}

}
