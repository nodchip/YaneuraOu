﻿#include "../shogi.h"

// KKPTの実験用コード。
// KPPTの評価関数ファイルをそのまま読み込む。
// KPPは手番をクリアする。
// ほとんどevaluate_kppt.cppと同じ。

#ifdef EVAL_KKPT

#include <fstream>
#include <iostream>
#include <unordered_set>

#include "evaluate_kppt.h"
#include "../evaluate.h"
#include "../position.h"
#include "../misc.h"

using namespace std;

namespace Eval
{

// 評価関数パラメーター
#if defined (USE_SHARED_MEMORY_IN_EVAL) && defined(_WIN32)

	// 共有メモリ上に確保する場合。

	ValueKk(*kk_)[SQ_NB][SQ_NB];
	ValueKpp(*kpp_)[SQ_NB][fe_end][fe_end];
	ValueKkp(*kkp_)[SQ_NB][SQ_NB][fe_end];

#else

	// 通常の評価関数テーブル。

	ALIGNED(32) ValueKk kk[SQ_NB][SQ_NB];
	ALIGNED(32) ValueKpp kpp[SQ_NB][fe_end][fe_end];
	ALIGNED(32) ValueKkp kkp[SQ_NB][SQ_NB][fe_end];

#endif

	void init() {}

	// 評価関数ファイルを読み込む
	void load_eval_impl()
	{
		{
			// KK
			std::ifstream ifsKK(path_combine((string)Options["EvalDir"], KK_BIN), std::ios::binary);
			if (ifsKK) ifsKK.read(reinterpret_cast<char*>(kk), sizeof(kk));
			else goto Error;

			// KKP
			std::ifstream ifsKKP(path_combine((string)Options["EvalDir"], KKP_BIN), std::ios::binary);
			if (ifsKKP) ifsKKP.read(reinterpret_cast<char*>(kkp), sizeof(kkp));
			else goto Error;

			// KPP
			std::ifstream ifsKPP(path_combine((string)Options["EvalDir"], KPP_BIN), std::ios::binary);
			if (ifsKPP) ifsKPP.read(reinterpret_cast<char*>(kpp), sizeof(kpp));
			else goto Error;

#if 0
			// kppのp1==p2のところ、値はゼロとなっていること。(参照はするけど学習のときに使いたくないので)
			{
				const ValueKpp kpp_zero = { 0,0 };
				float sum = 0;
				for (auto sq : SQ)
					for (auto p = BONA_PIECE_ZERO; p < fe_end; ++p)
					{
						sum += abs(kpp[sq][p][p][0]) + abs(kpp[sq][p][p][1]);
						kpp[sq][p][p] = kpp_zero;
					}
				cout << "sum kp = " << sum << endl;
			}

#endif

#if 0
			// Aperyの評価関数バイナリ、kppのp=0のところでゴミが入っている。
			// 駒落ちなどではここを利用したいので0クリアすべき。
			{
				const ValueKpp kpp_zero = { 0,0 };
				for (auto sq : SQ)
					for (BonaPiece p1 = BONA_PIECE_ZERO; p1 < fe_end; ++p1)
					{
						kpp[sq][p1][0] = kpp_zero;
						kpp[sq][0][p1] = kpp_zero;
					}

				const ValueKkp kkp_zero = { 0,0 };
				for (auto sq1 : SQ)
					for (auto sq2 : SQ)
						kkp[sq1][sq2][0] = kkp_zero;

			}
#endif


			// KKPTなので手番をクリアしておく。

#if 1
			// Aperyの評価関数バイナリ、kkptは意味があるけどkpptはあまり意味がないので
			// 手番価値をクリアする実験用のコード

			for (auto sq : SQ)
				for (BonaPiece p1 = BONA_PIECE_ZERO; p1 < fe_end; ++p1)
					for (BonaPiece p2 = BONA_PIECE_ZERO; p2 < fe_end; ++p2)
						kpp[sq][p1][p2][1] = 0;
#endif

#if 0
			// KPPTをPPTで代替えできないかを検証するためのコード
			// KPPTの値を平均化してPPTとして代入する。

			for (BonaPiece p1 = BONA_PIECE_ZERO; p1 < fe_end; ++p1)
				for (BonaPiece p2 = BONA_PIECE_ZERO; p2 < fe_end; ++p2)
				{
					int sum = 0;
					for (auto sq : SQ)
						sum += kpp[sq][p1][p2][1];

					int z = sum / SQ_NB;

					for (auto sq : SQ)
						kpp[sq][p1][p2][1] = z;
				}
#endif

#if 0
			// KPPTの手駒は手番必要ないのではないかを検証するためのコード
			for (auto sq : SQ)
				for (BonaPiece p1 = BONA_PIECE_ZERO; p1 < fe_hand_end; ++p1)
					for (BonaPiece p2 = BONA_PIECE_ZERO; p2 < fe_end; ++p2)
					{
							kpp[sq][p1][p2][1] = 0;
							kpp[sq][p2][p1][1] = 0;
					}
#endif

#if 0
			// どんな値がついているのかダンプして観察する用。
			for (BonaPiece p1 = BONA_PIECE_ZERO; p1 < fe_hand_end; ++p1)
				for (BonaPiece p2 = BONA_PIECE_ZERO; p2 < fe_end; ++p2)
				{
					cout << p1 << "," << p2 << " = " << (int)kpp[SQ_88][p1][p2][1] << endl;
				}
#endif

		}

		// 読み込みは成功した。

		return;

	Error:;
		// 評価関数ファイルの読み込みに失敗した場合、思考を開始しないように抑制したほうがいいと思う。
		sync_cout << "\ninfo string Error! open evaluation file failed.\n" << sync_endl;
		sleep(1000); // 出力される前に終了するのはまずいのでwaitを入れておく。
		exit(EXIT_FAILURE);
	}


	u64 calc_check_sum()
	{
		u64 sum = 0;
		
		auto add_sum = [&](u32*ptr , size_t t)
		{
			for (size_t i = 0; i < t; ++i)
				sum += ptr[i];
		};
		
		add_sum(reinterpret_cast<u32*>(kk) , sizeof(kk ) / sizeof(u32));
		add_sum(reinterpret_cast<u32*>(kkp), sizeof(kkp) / sizeof(u32));
		add_sum(reinterpret_cast<u32*>(kpp), sizeof(kpp) / sizeof(u32));

		return sum;
	}


#if defined (USE_SHARED_MEMORY_IN_EVAL) && defined(_WIN32)
	// 評価関数の共有を行うための大掛かりな仕組み
	// gccでコンパイルするときもWindows環境であれば、これが有効になって欲しいので defined(_WIN32) で判定。

#include <windows.h>
#include <codecvt>	// mkdirするのにwstringが欲しいのでこれが必要
#include <locale>   // wstring_convertにこれが必要。

	void load_eval()
	{
		// 評価関数を共有するのか
		if (!(bool)Options["EvalShare"])
		{
			// このメモリは、プロセス終了のときに自動開放されることを期待している。
			auto shared_eval_ptr = new SharedEval();

			if (shared_eval_ptr == nullptr)
			{
				sync_cout << "info string can't allocate eval memory." << sync_endl;
			}
			else
			{
				kk_ = &(shared_eval_ptr->kk_);
				kkp_ = &(shared_eval_ptr->kkp_);
				kpp_ = &(shared_eval_ptr->kpp_);

				load_eval_impl();
				// 共有されていないメモリを用いる。
				sync_cout << "info string use non-shared eval_memory." << sync_endl;
			}
			return;
		}

		// エンジンのバージョンによって評価関数は一意に定まるものとする。
		// Numaで確保する名前を変更しておく。

		auto dir_name = (string)Options["EvalDir"];
		// Mutex名にbackslashは使えないらしいので、escapeする。念のため'/'もescapeする。
		replace(dir_name.begin(), dir_name.end(), '\\', '_');
		replace(dir_name.begin(), dir_name.end(), '/', '_');
		// wchar_t*が必要なので変換する。
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> cv;
		cv.from_bytes(dir_name).c_str();

		auto mapped_file_name = TEXT("YANEURAOU_KPPT_MMF" ENGINE_VERSION) + cv.from_bytes(dir_name);
		auto mutex_name = TEXT("YANEURAOU_KPPT_MUTEX" ENGINE_VERSION) + cv.from_bytes(dir_name);

		// プロセス間の排他用mutex
		auto hMutex = CreateMutex(NULL, FALSE, mutex_name.c_str());

		// ファイルマッピングオブジェクトの処理をプロセス間で排他したい。
		WaitForSingleObject(hMutex, INFINITE);
		{

			// ファイルマッピングオブジェクトの作成
			auto hMap = CreateFileMapping(INVALID_HANDLE_VALUE,
				NULL,
				PAGE_READWRITE, // | /**SEC_COMMIT/**/ /*SEC_RESERVE/**/,
				0, sizeof(SharedEval),
				mapped_file_name.c_str());

			bool already_exists = (GetLastError() == ERROR_ALREADY_EXISTS);

			// ビュー
			auto shared_eval_ptr = (SharedEval *)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedEval));

			// メモリが確保できないときはshared_eval_ptr == null。このチェックをしたほうがいいような..。
			if (shared_eval_ptr == nullptr)
			{
				sync_cout << "info string can't allocate shared eval memory." << sync_endl;
			}
			else
			{
				kk_ = &(shared_eval_ptr->kk_);
				kkp_ = &(shared_eval_ptr->kkp_);
				kpp_ = &(shared_eval_ptr->kpp_);

				if (!already_exists)
				{
					// 新規作成されてしまった

					// このタイミングで評価関数バイナリを読み込む
					load_eval_impl();

					sync_cout << "info string created shared eval memory." << sync_endl;

				}
				else {

					// 評価関数バイナリを読み込む必要はない。ファイルマッピングが成功した時点で
					// 評価関数バイナリは他のプロセスによって読み込まれていると考えられる。

					sync_cout << "info string use shared eval memory." << sync_endl;
				}
			}
		}
		ReleaseMutex(hMutex);

		// 終了時に本当ならば
		// 1) ::ReleaseMutex()
		// 2) ::UnmapVieOfFile()
		// が必要であるが、1),2)がプロセスが解体されるときに自動でなされるので、この処理は特に入れない。

#else

	// 評価関数のプロセス間共有を行わないときは、普通に
	// load_eval_impl()を呼び出すだけで良い。
	void load_eval()
	{
		load_eval_impl();
#endif

		// 共有メモリを使う/使わないときの共通の処理
		auto check_sum = calc_check_sum();

		// 評価関数ファイルの正体
		string softname = "unknown";

		// ソフト名自動判別
		map<u64, string> list = {
			{ 0x7171a5469027ebf , "ShinYane(20161010)" } ,
			{ 0x71fc7fd40c668cc , "Ukamuse(sdt4)" } ,

			{ 0x65cd7c55a9d4cd9 , "elmo(WCSC27)" } ,
			{ 0x3aa68b055a020a8 , "Yomita(WCSC27)" } ,
			{ 0x702fb2ee5672156 , "Qhapaq(WCSC27)" } ,
			{ 0x6c54a1bcb654e37 , "tanuki(WCSC27)" } ,
		};
		if (list.count(check_sum))
			softname = list[check_sum];

		sync_cout << "info string Eval Check Sum = " << std::hex << check_sum << std::dec
				  << " , Eval File = " << softname << sync_endl;
	}

	// KP,KPP,KKPのスケール
	const int FV_SCALE = 32;

	// 駒割り以外の全計算
	// pos.st->BKPP,WKPP,KPPを初期化する。Position::set()で一度だけ呼び出される。(以降は差分計算)
	// 手番側から見た評価値を返すので注意。(他の評価関数とは設計がこの点において異なる)
	// なので、この関数の最適化は頑張らない。
	Value compute_eval(const Position& pos)
	{
#if defined (USE_SHARED_MEMORY_IN_EVAL) && defined(_WIN32)
		// shared memoryを用いているときには、is_ready()で評価関数を読み込み、
		// 初期化してからしかcompute_eval()を呼び出すことは出来ない。
		ASSERT_LV1(kk_ != nullptr);
#endif

		Square sq_bk = pos.king_square(BLACK);
		Square sq_wk = pos.king_square(WHITE);
		const auto* ppkppb = kpp[sq_bk];
		const auto* ppkppw = kpp[Inv(sq_wk)];

		auto& pos_ = *const_cast<Position*>(&pos);

		auto list_fb = pos_.eval_list()->piece_list_fb();
		auto list_fw = pos_.eval_list()->piece_list_fw();

		int i, j;
		BonaPiece k0, k1, l0, l1;

		// 評価値の合計
		EvalSum sum;


#if defined(USE_SSE2)
		// sum.p[0](BKPP)とsum.p[1](WKPP)をゼロクリア
		sum.m[0] = _mm_setzero_si128();
#else
		sum.p[0][0] = sum.p[0][1] = sum.p[1][0] = sum.p[1][1] = 0;
#endif

		// KK
		sum.p[2] = kk[sq_bk][sq_wk];

		for (i = 0; i < PIECE_NO_KING; ++i)
		{
			k0 = list_fb[i];
			k1 = list_fw[i];
			const auto* pkppb = ppkppb[k0];
			const auto* pkppw = ppkppw[k1];
			for (j = 0; j < i; ++j)
			{
				l0 = list_fb[j];
				l1 = list_fw[j];

				// KPPの手番は無視して加算
				sum.p[0][0] += pkppb[l0][0];
				sum.p[1][0] += pkppw[l1][0];
			}
			sum.p[2] += kkp[sq_bk][sq_wk][k0];
		}

		auto st = pos.state();
		sum.p[2][0] += st->materialValue * FV_SCALE;

		st->sum = sum;

		return Value(sum.sum(pos.side_to_move()) / FV_SCALE);
	}

	// 先手玉が移動したときに先手側の差分
	std::array<s32, 2> do_a_black(const Position& pos, const ExtBonaPiece ebp) {
		const Square sq_bk = pos.king_square(BLACK);
		const auto* list0 = pos.eval_list()->piece_list_fb();

		const auto* pkppb = kpp[sq_bk][ebp.fb];

		// 手番であるsum[1]は0にして返す。
		std::array<s32, 2> sum = { { pkppb[list0[0]][0], 0 } };
		for (int i = 1; i < PIECE_NO_KING; ++i) {
			sum[0] += pkppb[list0[i]][0];
		}
		return sum;
	}

	// 後手玉が移動したときの後手側の差分
	std::array<s32, 2> do_a_white(const Position& pos, const ExtBonaPiece ebp) {
		const Square sq_wk = pos.king_square(WHITE);
		const auto* list1 = pos.eval_list()->piece_list_fw();

		const auto* pkppw = kpp[Inv(sq_wk)][ebp.fw];
		std::array<s32, 2> sum = { { pkppw[list1[0]][0], 0 } };
		for (int i = 1; i < PIECE_NO_KING; ++i) {
			sum[0] += pkppw[list1[i]][0];
		}
		return sum;
	}

	// 玉以外の駒が移動したときの差分
	EvalSum do_a_pc(const Position& pos, const ExtBonaPiece ebp) {
		const Square sq_bk = pos.king_square(BLACK);
		const Square sq_wk = pos.king_square(WHITE);
		const auto list0 = pos.eval_list()->piece_list_fb();
		const auto list1 = pos.eval_list()->piece_list_fw();

		EvalSum sum;
		sum.p[0] = { 0, 0 };
		sum.p[1] = { 0, 0 };
		sum.p[2] = kkp[sq_bk][sq_wk][ebp.fb];

		const auto* pkppb = kpp[sq_bk][ebp.fb];
		const auto* pkppw = kpp[Inv(sq_wk)][ebp.fw];

		sum.p[0][0] = pkppb[list0[0]][0];
//		sum.p[0][1] = pkppb[list0[0]][1];
		sum.p[1][0] = pkppw[list1[0]][0];
//		sum.p[1][1] = pkppw[list1[0]][1];
		for (int i = 1; i < PIECE_NO_KING; ++i) {
			sum.p[0][0] += pkppb[list0[i]][0];
			sum.p[1][0] += pkppw[list1[i]][0];
		}

		return sum;
	}


#ifdef USE_EVAL_HASH
	EvaluateHashTable g_evalTable;

	// prefetchする関数も用意しておく。
	void prefetch_evalhash(const Key key)
	{
		prefetch(g_evalTable[key >> 1]);
	}

#endif

	void evaluateBody(const Position& pos)
	{
		// 過去に遡って差分を計算していく。

		auto now = pos.state();
		auto prev = now->previous;

		// nodeごとにevaluate()は呼び出しているので絶対に差分計算できるはず。
		// 一つ前のnodeでevaluate()されているはず。
		//
		// root nodeではprevious == nullptrであるが、root nodeではPosition::set()でcompute_eval()
		// を呼び出すので通常この関数が呼び出されることはないのだが、学習関係でこれが出来ないと
		// コードが書きにくいのでEVAL_LEARNのときは、このチェックをする。
		if (
#ifdef EVAL_LEARN
			prev == nullptr ||
#endif
			!prev->sum.evaluated())
		{
			// 全計算
			compute_eval(pos);
			return;
			// 結果は、pos->state().sumから取り出すべし。
		}

		// 遡るnodeは一つだけ
		// ひとつずつ遡りながらsumKPPがVALUE_NONEでないところまで探してそこからの差分を計算することは出来るが
		// 現状、探索部では毎node、evaluate()を呼び出すから問題ない。

		auto& dp = now->dirtyPiece;

		// 移動させた駒は最大2つある。その数
		int moved_piece_num = dp.dirty_num;

		auto list0 = pos.eval_list()->piece_list_fb();
		auto list1 = pos.eval_list()->piece_list_fw();

		auto dirty = dp.pieceNo[0];

		// 移動させた駒は王か？
		if (dirty >= PIECE_NO_KING)
		{
			// 前のnodeの評価値からの増分を計算していく。
			// (直接この変数に加算していく)
			// この意味においてdiffという名前は少々不適切ではあるが。
			EvalSum diff = prev->sum;

			auto sq_bk = pos.king_square(BLACK);
			auto sq_wk = pos.king_square(WHITE);

			// ΣKKPは最初から全計算するしかないので初期化する。
			diff.p[2] = kk[sq_bk][sq_wk];
			diff.p[2][0] += now->materialValue * FV_SCALE;

			// 後手玉の移動(片側分のKPPを丸ごと求める)
			if (dirty == PIECE_NO_WKING)
			{
				const auto ppkppw = kpp[Inv(sq_wk)];

				// ΣWKPP = 0
				diff.p[1][0] = 0;
				diff.p[1][1] = 0;

				for (int i = 0; i < PIECE_NO_KING; ++i)
				{
					const int k1 = list1[i];
					const auto* pkppw = ppkppw[k1];
					for (int j = 0; j < i; ++j)
					{
						const int l1 = list1[j];
						// 手番を無視して[0]だけ更新
						diff.p[1][0] += pkppw[l1][0];
					}

					// KKPのWK分。BKは移動していないから、BK側には影響ない。

					// 後手から見たKKP。後手から見ているのでマイナス
					diff.p[2][0] -= kkp[Inv(sq_wk)][Inv(sq_bk)][k1][0];
					// 後手から見たKKP手番。後手から見るのでマイナスだが、手番は先手から見たスコアを格納するのでさらにマイナスになって、プラス。
					diff.p[2][1] += kkp[Inv(sq_wk)][Inv(sq_bk)][k1][1];
				}

				// 動かした駒が２つ
				if (moved_piece_num == 2)
				{
					// 瞬間的にeval_listの移動させた駒の番号を変更してしまう。
					// こうすることで前nodeのpiece_listを持たなくて済む。

					const int listIndex_cap = dp.pieceNo[1];
					diff.p[0] += do_a_black(pos, dp.changed_piece[1].new_piece);
					list0[listIndex_cap] = dp.changed_piece[1].old_piece.fb;
					diff.p[0] -= do_a_black(pos, dp.changed_piece[1].old_piece);
					list0[listIndex_cap] = dp.changed_piece[1].new_piece.fb;
				}

			} else {

				// 先手玉の移動
				// さきほどの処理と同様。

				const auto* ppkppb = kpp[sq_bk];
				diff.p[0][0] = 0;
				diff.p[0][1] = 0;

				if (moved_piece_num == 2) {
					const int listIndex_cap = dp.pieceNo[1];
					diff.p[1] += do_a_white(pos, dp.changed_piece[1].new_piece);
					list1[listIndex_cap] = dp.changed_piece[1].old_piece.fw;
					diff.p[1] -= do_a_white(pos, dp.changed_piece[1].old_piece);
					list1[listIndex_cap] = dp.changed_piece[1].new_piece.fw;
				}
			}

			// sumの計算が終わったのでpos.state()->sumに反映させておく。(これがこの関数の返し値に相当する。)
			now->sum = diff;

		} else {

			// 王以外の駒が移動したケース
			// 今回の差分を計算して、そこに加算する。

			const int listIndex = dp.pieceNo[0];

			auto diff = do_a_pc(pos, dp.changed_piece[0].new_piece);
			if (moved_piece_num == 1) {

				// 動いた駒が1つ。
				list0[listIndex] = dp.changed_piece[0].old_piece.fb;
				list1[listIndex] = dp.changed_piece[0].old_piece.fw;
				diff -= do_a_pc(pos, dp.changed_piece[0].old_piece);

			} else {

				// 動いた駒が2つ。

				auto sq_bk = pos.king_square(BLACK);
				auto sq_wk = pos.king_square(WHITE);

				diff += do_a_pc(pos, dp.changed_piece[1].new_piece);
				diff.p[0][0] -= kpp[sq_bk][dp.changed_piece[0].new_piece.fb][dp.changed_piece[1].new_piece.fb][0];
				diff.p[1][0] -= kpp[Inv(sq_wk)][dp.changed_piece[0].new_piece.fw][dp.changed_piece[1].new_piece.fw][0];

				const PieceNo listIndex_cap = dp.pieceNo[1];
				list0[listIndex_cap] = dp.changed_piece[1].old_piece.fb;
				list1[listIndex_cap] = dp.changed_piece[1].old_piece.fw;

				list0[listIndex] = dp.changed_piece[0].old_piece.fb;
				list1[listIndex] = dp.changed_piece[0].old_piece.fw;
				diff -= do_a_pc(pos, dp.changed_piece[0].old_piece);
				diff -= do_a_pc(pos, dp.changed_piece[1].old_piece);

				diff.p[0][0] += kpp[sq_bk][dp.changed_piece[0].old_piece.fb][dp.changed_piece[1].old_piece.fb][0];
				diff.p[1][0] += kpp[Inv(sq_wk)][dp.changed_piece[0].old_piece.fw][dp.changed_piece[1].old_piece.fw][0];
				list0[listIndex_cap] = dp.changed_piece[1].new_piece.fb;
				list1[listIndex_cap] = dp.changed_piece[1].new_piece.fw;
			}

			list0[listIndex] = dp.changed_piece[0].new_piece.fb;
			list1[listIndex] = dp.changed_piece[0].new_piece.fw;

			// 前nodeからの駒割りの増分を加算。
			diff.p[2][0] += (now->materialValue - prev->materialValue) * FV_SCALE;

			now->sum = diff + prev->sum;
		}

	}

	// 評価関数
	Value evaluate(const Position& pos)
	{
		auto st = pos.state();
		auto &sum = st->sum;

		// すでに計算済(Null Moveなどで)であるなら、それを返す。
		if (sum.evaluated())
			return Value(sum.sum(pos.side_to_move()) / FV_SCALE);

#ifdef USE_EVAL_HASH
		// evaluate hash tableにはあるかも。

		// 手番を消した局面hash key
		const Key keyExcludeTurn = st->key() >> 1;
		EvalSum entry = *g_evalTable[keyExcludeTurn];   // atomic にデータを取得する必要がある。
		entry.decode();
		if (entry.key == keyExcludeTurn)
		{
//			dbg_hit_on(true);
			// あった！
			sum = entry;
			return Value(entry.sum(pos.side_to_move()) / FV_SCALE);
		}
//		dbg_hit_on(false);
#endif

		// 評価関数本体を呼び出して求める。
		evaluateBody(pos);

#ifdef USE_EVAL_HASH
		// せっかく計算したのでevaluate hash tableに保存しておく。
		sum.key = keyExcludeTurn;
		sum.encode();
		*g_evalTable[keyExcludeTurn] = sum;
#endif

		ASSERT_LV5(pos.state()->materialValue == Eval::material(pos));
		// 差分計算と非差分計算との計算結果が合致するかのテスト。(さすがに重いのでコメントアウトしておく)
		// ASSERT_LV5(Value(st->sum.sum(pos.side_to_move()) / FV_SCALE) == compute_eval(pos));

#if 0
		if (!(Value(st->sum.sum(pos.side_to_move()) / FV_SCALE) == compute_eval(pos)))
		{
			st->sum.p[0][0] = VALUE_NOT_EVALUATED;
			evaluateBody(pos);
		}
#endif

		return Value(sum.sum(pos.side_to_move()) / FV_SCALE);
	}

	void evaluate_with_no_return(const Position& pos)
	{
		// まだ評価値が計算されていないなら
		if (!pos.state()->sum.evaluated())
			evaluate(pos);
	}

	// 現在の局面の評価値の内訳を表示する。
	void print_eval_stat(Position& pos)
	{
		cout << "--- EVAL STAT\n";

		Square sq_bk = pos.king_square(BLACK);
		Square sq_wk = pos.king_square(WHITE);
		const auto* ppkppb = kpp[sq_bk];
		const auto* ppkppw = kpp[Inv(sq_wk)];

		auto& pos_ = *const_cast<Position*>(&pos);

		auto list_fb = pos_.eval_list()->piece_list_fb();
		auto list_fw = pos_.eval_list()->piece_list_fw();

		int i, j;
		BonaPiece k0, k1, l0, l1;

		// 38枚の駒を表示
		for (i = 0; i < PIECE_NO_KING; ++i)
			cout << int(list_fb[i]) << " = " << list_fb[i] << " , " << int(list_fw[i]) << " =  " << list_fw[i] << endl;

		// 評価値の合計
		EvalSum sum;

#if defined(USE_SSE2)
		// sum.p[0](BKPP)とsum.p[1](WKPP)をゼロクリア
		sum.m[0] = _mm_setzero_si128();
#else
		sum.p[0][0] = sum.p[0][1] = sum.p[1][0] = sum.p[1][1] = 0;
#endif

		// KK
		sum.p[2] = kk[sq_bk][sq_wk];
		cout << "KKC : " << sq_bk << " " << sq_wk << " = " << kk[sq_bk][sq_wk][0] << " + " << kk[sq_bk][sq_wk][1] << "\n";

		for (i = 0; i < PIECE_NO_KING; ++i)
		{
			k0 = list_fb[i];
			k1 = list_fw[i];
			const auto* pkppb = ppkppb[k0];
			const auto* pkppw = ppkppw[k1];
			for (j = 0; j < i; ++j)
			{
				l0 = list_fb[j];
				l1 = list_fw[j];

#if defined(USE_SSE41)
				// SSEによる実装

				// pkppw[l1][0],pkppw[l1][1],pkppb[l0][0],pkppb[l0][1]の16bit変数4つを整数拡張で32bit化して足し合わせる
				__m128i tmp;
				tmp = _mm_set_epi32(0, 0, *reinterpret_cast<const int32_t*>(&pkppw[l1][0]), *reinterpret_cast<const int32_t*>(&pkppb[l0][0]));
				// この命令SSE4.1の命令のはず
				tmp = _mm_cvtepi16_epi32(tmp);
				sum.m[0] = _mm_add_epi32(sum.m[0], tmp);

				cout << "BKPP : " << sq_bk << " " << k0 << " " << l0 << " = " << pkppb[l0][0] << " + " << pkppb[l0][1] << "\n";
				cout << "WKPP : " << sq_wk << " " << k1 << " " << l1 << " = " << pkppw[l1][0] << " + " << pkppw[l1][1] << "\n";

#else
				sum.p[0] += pkppb[l0];
				sum.p[1] += pkppw[l1];
#endif
			}
			sum.p[2] += kkp[sq_bk][sq_wk][k0];

			cout << "KKP : " << sq_bk << " " << sq_wk << " " << k0 << " = " << kkp[sq_bk][sq_wk][k0][0] << " + " << kkp[sq_bk][sq_wk][k0][1] << "\n";

		}

		cout << "Material = " << pos.state()->materialValue << endl;
		cout << sum;
		cout << "---\n";

	}

}

#endif // EVAL_KKPT
