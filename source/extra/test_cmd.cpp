﻿#include "../shogi.h"

// USI拡張コマンドのうち、開発上のテスト関係のコマンド。
// 思考エンジンの実行には関係しない。GitHubにはcommitしないかも。

#ifdef ENABLE_TEST_CMD

#include "all.h"
#include <unordered_set>

extern void is_ready();

// ----------------------------------
//  USI拡張コマンド "perft"(パフォーマンステスト)
// ----------------------------------

// perft()で用いるsolver
// cf. http://qiita.com/ak11/items/8bd5f2bb0f5b014143c8

// 通常のPERFTと、置換表を用いる高速なPERFTと選択できる。
// 後者を用いる場合は、hash keyの衝突を避けるためにHASH_KEY_BITSを128にしておくこと。
// ※　あと、後者は以下のところで置換表を15GBほど固定で確保しているので、動作環境に応じて修正すること。
// >  entryCount = 256 * 1024 * 1024; // * sizeof(PerftSolverResult) == 15GBほど

#define NORMAL_PERFT

// perftのときにeval値も加算していくモード。評価関数のテスト用。
//#define EVAL_PERFT


struct PerftSolverResult {
  uint64_t nodes, captures, promotions, checks, mates;
#ifdef EVAL_PERFT
  int64_t eval;
#endif

  void operator+=(const PerftSolverResult& other) {
    nodes += other.nodes;
    captures += other.captures;
    promotions += other.promotions;
    checks += other.checks;
    mates += other.mates;
#ifdef EVAL_PERFT
    eval += other.eval;
#endif
  }
};

#ifndef NORMAL_PERFT
// perftで用いる置換表
namespace Perft {
  struct TTEntry {
    void save(HASH_KEY key_, PerftSolverResult& result_)
    {
      key = key_;
      result = result_;
    }
    HASH_KEY key;
    PerftSolverResult result;
  };

  struct TranspositionTable {
    TranspositionTable() {
      entryCount = 256*1024*1024; // * sizeof(PerftSolverResult) == 15GBほど
      table = (TTEntry*)calloc( entryCount * sizeof(TTEntry) , 1);
    }
    ~TranspositionTable() { free(table); }

    TTEntry* probe(const HASH_KEY key_,int depth,bool& found)
    {
      auto key = key_ ^ DepthHash(depth); // depthの分だけhash keyを変更しておく。
      auto& tte = table[key /*.p(0)*/ & (entryCount - 1)];
      found = (tte.key == key_); // 変更前のhash keyが書かれているはず
      return &tte;
    }
  private:
    TTEntry* table;
    size_t entryCount; // TTEntryの数
  };
  TranspositionTable TT;
}

#endif

#ifdef NORMAL_PERFT
struct PerftSolver {
  template <bool Root>
  PerftSolverResult Perft(Position& pos, int depth) {
    StateInfo st;
    PerftSolverResult result = {};
    if (depth == 0) {
      result.nodes++;
      if (pos.captured_piece() != NO_PIECE) result.captures++;
#ifdef KEEP_LAST_MOVE
      if (is_promote(pos.state()->lastMove)) result.promotions++;
#endif
#ifdef EVAL_PERFT
//      cout << pos.sfen() << " , eval = " << Eval::evaluate(pos) << endl;
      /*
      if (pos.sfen() == "1nsgkgsnl/lr5b1/pppppp+Bpp/9/9/2P6/PP1PPPPPP/7R1/LNSGKGSNL w P 4")
      {
//        cout << Eval::evaluate(pos);
        Eval::print_eval_stat(pos);
      }
      */
      result.eval += Eval::evaluate(pos);
#endif
      if (pos.checkers()) {
        result.checks++;
        if (pos.is_mated()) result.mates++;
      }
    } else {
      for (auto m : MoveList<LEGAL_ALL>(pos)) {
        if (Root)
          cout << ".";
        pos.do_move(m, st);
        result += Perft<false>(pos, depth - 1);
        pos.undo_move(m);
      }
    }
    return result;
  }
};
#else // 置換表を用いる高速なperft
struct PerftSolver {
  template <bool Root>
  PerftSolverResult Perft(Position& pos, int depth) {
    HASH_KEY key = pos.state()->long_key();
    bool found;

    PerftSolverResult result = {};

    auto tt = Perft::TT.probe(key, depth, found); // 置換表に登録されているか。
    if (found)
      return tt->result;

    StateInfo st;
    for (auto m : MoveList<LEGAL_ALL>(pos)) {
      if (Root)
        cout << ".";

      pos.do_move(m.move, st);
      if (depth > 1)
        result += Perft<false>(pos, depth - 1);
      else {
        result.nodes++;
        if (pos.state()->capturedType != NO_PIECE) result.captures++;
        #ifdef        KEEP_LAST_MOVE
        if (is_promote(pos.state()->lastMove)) result.promotions++;
        #endif
        if (pos.checkers()) {
          result.checks++;
          if (pos.is_mated()) result.mates++;
        }
      }
      pos.undo_move(m.move);
    }
    tt->save(key, result); // 登録されていなかったので置換表に保存する

    return result;
  }
};
#endif

// N手で到達できる局面数を計算する。成る手、取る手、詰んだ局面がどれくらい含まれているかも計算する。
void perft(Position& pos, istringstream& is)
{
  int depth = 5 ;
  is >> depth;
  cout << "perft depth = " << depth << endl;
  PerftSolver solver;
  // 局面コピーして並列的に呼び出してやるだけであとはなんとかなる。

  auto result = solver.Perft<true>(pos, depth);

  cout << endl << "nodes = " << result.nodes << " , captures = " << result.captures <<
#ifdef KEEP_LAST_MOVE
    " , promotion = " << result.promotions <<
#endif
#ifdef EVAL_PERFT
    " , eval(sum) = " << result.eval <<
#endif
    " , checks = " << result.checks << " , checkmates = " << result.mates << endl;
}

// ----------------------------------
//      USI拡張コマンド "test"
// ----------------------------------

// 利きの整合性のチェック
void effect_check(Position& pos)
{
#if defined(LONG_EFFECT_LIBRARY) && defined(KEEP_LAST_MOVE)
  // 利きは、Position::set_effect()で全計算され、do_move()のときに差分更新されるが、
  // 差分更新された値がset_effect()の値と一致するかをテストする。
  using namespace LongEffect;
  ByteBoard bb[2] = { pos.board_effect[0] , pos.board_effect[1] };
  WordBoard wb = pos.long_effect;

  LongEffect::calc_effect(pos);
  
  for(auto c : COLOR)
    for(auto sq : SQ)
      if (bb[c].effect(sq) != pos.board_effect[c].effect(sq))
      {
        cout << "Error effect count of " << c << " at " << sq << endl << pos << "wrong\n" << bb[c] << endl << "correct\n" << pos.board_effect[c] << pos.moves_from_start_pretty();
        ASSERT(false);
      }

  for(auto sq : SQ)
    if (wb.long_effect16(sq) != pos.long_effect.long_effect16(sq))
    {
      cout << "Error long effect at " << sq << endl << pos << "wrong\n" << wb << endl << "correct\n" << pos.long_effect << pos.moves_from_start_pretty();
      ASSERT(false);
    }

#endif
}


// --- "test rp"コマンド

// ランダムプレイヤーで行なうテストの種類

// 利きの整合性のテスト
//#define EFFECT_CHECK

// 1手詰め判定のテスト
// #define MATE1PLY_CHECK

// 評価関数の差分計算等のチェック
//#define EVAL_VALUE_CHECK


void random_player(Position& pos,uint64_t loop_max)
{
#ifdef MATE1PLY_CHECK
  uint64_t mate_found = 0;    // 1手詰め判定で見つけた1手詰め局面の数 
  uint64_t mate_missed = 0;   // 1手詰め判定で見逃した1手詰め局面の数
#endif

  pos.set_hirate();
  const int MAX_PLY = 256; // 256手までテスト

  StateInfo state[MAX_PLY]; // StateInfoを最大手数分だけ
  Move moves[MAX_PLY]; // 局面の巻き戻し用に指し手を記憶
  int ply; // 初期局面からの手数

  PRNG prng(20160101);
  
  for (uint64_t i = 0; i < loop_max; ++i)
  {
    for (ply = 0; ply < MAX_PLY; ++ply)
    {
      MoveList<LEGAL_ALL> mg(pos); // 全合法手の生成

      // 合法な指し手がなかった == 詰み
      if (mg.size() == 0)
        break;

      // 局面がおかしくなっていないかをテストする
      ASSERT_LV3(is_ok(pos));

#ifdef EVAL_VALUE_CHECK
      {
        // 評価値の差分計算等がsfen文字列をセットしての全計算と一致するかのテスト(すこぶる遅い)
        auto value = Eval::eval(pos);
        pos.set(pos.sfen());
        auto value2 = Eval::eval(pos);
        ASSERT_LV3(value == value2);
      }
#endif

      // ここで生成された指し手がすべて合法手であるかテストをする
      for (auto m : mg)
      {
        ASSERT_LV3(pos.pseudo_legal(m));
        ASSERT_LV2(pos.legal(m));
      }

#ifdef MATE1PLY_CHECK
      {
        // 王手のかかっていない局面においてテスト
        if (!pos.in_check())
        {
          Move m = pos.mate1ply();
          if (m != MOVE_NONE)
          {
  //          cout << pos << m;
            m = pos.move16_to_move(m);

            if (!pos.pseudo_legal(m) || !pos.legal(m))
            {
              cout << endl << pos << "not legal , mate1ply() = " << m << endl;
              m = pos.mate1ply();
              ASSERT(false);
            }

            // これで本当に詰んでいるのかテストする
            pos.do_move(m, state[ply]);
            if (!pos.is_mated())
            {
              pos.undo_move(m);
              // 局面を戻してから指し手を表示しないと目視で確認できない。
              cout << endl << pos << "not mate , mate1ply() = " << m << endl;
              m = pos.mate1ply();
              ASSERT(false);
            } else {
              pos.undo_move(m);
//              cout << "M"; // mateだったときにこれを表示してpassした個数のチェック
              ++mate_found;

              // 統計情報の表示
              if ((mate_found % 10000) == 0)
              {
                cout << endl << "mate found = " << mate_found << " , mate miss = " << mate_missed
                  << " , mate found rate  = " << double(100*mate_found) / (mate_found + mate_missed) << "%"<< endl;
              }
            }
          } else {

            // 1手詰め判定で漏れた局面を探す
            for (auto m : MoveList<LEGAL_ALL>(pos))
            {
              pos.do_move(m, state[ply]);
              if (pos.is_mated())
              {
                pos.undo_move(m);
                // 局面が表示されすぎて統計情報がわかりにくいときはコメントアウトする
//                cout << endl << pos << "mated = " << m.move << ", but mate1ply() = MOVE_NONE." << endl;
                pos.mate1ply();

                ++mate_missed;
                break;
              }
              pos.undo_move(m);
            }

          }
        }
      }
#endif

      // 生成された指し手のなかからランダムに選び、その指し手で局面を進める。
      Move m = mg.begin()[prng.rand(mg.size())];

      pos.do_move(m, state[ply]);
      moves[ply] = m;

#ifdef EFFECT_CHECK
      // 利きの整合性のテスト(重いのでテストが終わったらコメントアウトする)
      effect_check(pos);
#endif
    }

#ifdef EVAL_VALUE_CHECK
    pos.set_hirate(); // Position.set()してしまったので巻き戻せない
#else
    // 局面を巻き戻してみる(undo_moveの動作テストを兼ねて)
    while (ply > 0)
    {
      pos.undo_move(moves[--ply]);

#ifdef EFFECT_CHECK
      // 利きの整合性のテスト(重いのでテストが終わったらコメントアウトする)
      effect_check(pos);
#endif
    }

#endif

    // 1000回に1回ごとに'.'を出力(進んでいることがわかるように)
    if ((i % 1000) == 0)
      cout << ".";
  }
}

// ランダムプレイヤー(指し手をランダムに選ぶプレイヤー)による自己対戦テスト
// これを1000万回ほどまわせば、指し手生成などにバグがあればどこかで引っかかるはず。

void random_player_cmd(Position& pos, istringstream& is)
{
  uint64_t loop_max = 100000000; // 1億回
  is >> loop_max;
  cout << "Random Player test , loop_max = " << loop_max << endl;
  random_player(pos, loop_max);
  cout << "finished." << endl;
}


// --- "test rpbench"コマンド

// ランダムプレイヤーを用いたベンチマーク
void random_player_bench_cmd(Position& pos, istringstream& is)
{
  uint64_t loop_max = 50000; // default 5万回
  is >> loop_max;
  cout << "Random Player bench test , loop_max = " << loop_max << endl;

  pos.set_hirate();
  const int MAX_PLY = 256; // 256手までテスト

  StateInfo state[MAX_PLY]; // StateInfoを最大手数分だけ
  Move moves[MAX_PLY]; // 局面の巻き戻し用に指し手を記憶
  int ply; // 初期局面からの手数

  PRNG prng(20160123); // これ作った日

  auto start = now();

  for (uint64_t i = 0; i < loop_max; ++i)
  {
    for (ply = 0; ply < MAX_PLY; ++ply)
    {
      MoveList<LEGAL_ALL> mg(pos);
      if (mg.size() == 0)
        break;

      Move m = mg.begin()[prng.rand(mg.size())];

      pos.do_move(m, state[ply]);
      moves[ply] = m;
    }
    // 局面を巻き戻してみる(undo_moveの動作テストを兼ねて)
    while (ply > 0)
    {
      pos.undo_move(moves[--ply]);
    }
    // 1000回に1回ごとに'.'を出力(進んでいることがわかるように)
    if ((i % 1000) == 0)
      cout << ".";
  }

  auto end = now();
  auto gps = ((double)loop_max)*1000/(end - start);
  cout << endl << "bench done , " << gps << " games/second " << endl;
}


// --- "test genchecks"コマンド

// ランダムプレイヤーに合法手を生成させて、そのなかの王手になる指し手が
// 王手生成ルーチンで生成した指し手と合致するかを判定して、王手生成ルーチンの正しさを証明する。
void test_genchecks(Position& pos, uint64_t loop_max)
{
  pos.init();
  const int MAX_PLY = 256; // 256手までテスト

  StateInfo state[MAX_PLY]; // StateInfoを最大手数分だけ
  Move moves[MAX_PLY]; // 局面の巻き戻し用に指し手を記憶
  int ply; // 初期局面からの手数

  for (uint64_t i = 0; i < loop_max; ++i)
  {
    for (ply = 0; ply < MAX_PLY; ++ply)
    {
      MoveList<LEGAL_ALL> mg(pos); // 全合法手の生成

                               // 合法な指し手がなかった == 詰み
      if (mg.size() == 0)
        break;

      // 局面がおかしくなっていないかをテストする
      ASSERT_LV3(is_ok(pos));

      MoveList<CHECKS_ALL> mc(pos);

      // ここで生成された指し手と王手生成ルーチンで生成した指し手とが王手する指し手について一致するかをテストする。
      for (auto m : mg)
      {
        if (pos.gives_check(m))
        {
          for (auto m2 : mc)
            if (m2.move == m)
              goto Exit;

          cout << endl << pos << "not found : move = " << m.move << endl;
          MoveList<CHECKS_ALL> mc2(pos); // ここにブレークポイントを仕掛けてデバッグする。
          ASSERT_LV1(false);
        }
      Exit:;
      }

      // 逆もチェックする。
      for (auto m : mc)
      {
        if (!pos.gives_check(m))
        {
          cout << endl << pos << "not checks : move = " << m.move << endl;
          MoveList<CHECKS_ALL> mc2(pos); // ここにブレークポイントを仕掛けてデバッグする。
          ASSERT_LV1(false);
        }
      }


      // 生成された指し手のなかからランダムに選び、その指し手で局面を進める。
      Move m = mg.begin()[rand() % mg.size()];

      pos.do_move(m, state[ply]);
      moves[ply] = m;
    }
    // 局面を巻き戻してみる(undo_moveの動作テストを兼ねて)
    while (ply > 0)
      pos.undo_move(moves[--ply]);

    // 100回に1回ごとに'.'を出力(進んでいることがわかるように)
    if ((i % 1000) == 0)
      cout << ".";
  }
}

void test_genchecks(Position& pos, istringstream& is)
{
  uint64_t loop_max = 100000000; // 1000万回
  is >> loop_max;
  cout << "Generate Checks test , loop_max = " << loop_max << endl;
  test_genchecks(pos, loop_max);
  cout << "finished." << endl;
}


// --- "test cm"コマンド

// 協力詰め。n手で協力詰めで詰むかを調べる。

// 再帰的に呼び出される
// depth = 残り探索深さ
void cooperation_mate(Position& pos, int depth)
{
  // moves_from_start_pretty()がKEEP_LAST_MOVEを要求する。
#ifdef  KEEP_LAST_MOVE

  StateInfo st;
  for (auto m : MoveList<LEGAL_ALL>(pos))
  {
    pos.do_move(m.move, st);

    if (pos.is_mated())
    {
      // 後手の詰みなら手順を表示する。先手の詰みは必要ない。
      if (pos.side_to_move() == WHITE)
        cout << pos << pos.moves_from_start_pretty() << endl; // 開始局面からそこまでの手順
    } else {
      // 残り探索深さがあるなら再帰的に探索する。
      if (depth > 1)
        cooperation_mate(pos, depth - 1);
    }
    pos.undo_move(m.move);
  }
#endif
}

// 協力詰めコマンド
// 例)
//  position sfen 9/9/9/3bkb3/9/3+R1+R3/9/9/9 b - 1
//  position sfen 9/6b2/7k1/5b3/7sL/9/9/9/9 b R - 1
//  などで
//  test cm 5  (5手詰め)
void cooperation_mate_cmd(Position& pos, istringstream& is)
{
  int depth = 5;
  is >> depth;
  cout << "Cooperation Mate test , depth = " << depth << endl;
  cooperation_mate(pos, depth); // 与えられた局面からdepth深さの協力詰みを探す
  cout << "finished." << endl;
}

// --- "s" 指し手生成テストコマンド
void generate_moves_cmd(Position& pos)
{
  cout << "Generate Moves Test.." << endl;
//  pos.set("l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1");
  auto start = now();

  // 試行回数
  const int64_t n = 30000000;
  for (int i = 0; i < n; ++i)
  {
    if (pos.checkers())
      MoveList<EVASIONS> ml(pos);
    else
      MoveList<NON_EVASIONS> ml(pos);
  }

  auto end = now();

  // 局面と生成された指し手を出力しておく。
  cout << pos;

  if (pos.checkers())
    for (auto m : MoveList<EVASIONS>(pos))
      cout << m.move << ' ';
  else
    for (auto m : MoveList<NON_EVASIONS>(pos))
      cout << m.move << ' ';

  cout << endl << (1000 * n / (end - start)) << " times per second." << endl;
}

// --- 駒の優劣関係などのテスト

void test_hand()
{
  cout << "Test Hand start : is_superior()" << endl;
  
  for (int i = 0; i < 128; ++i)
    for (int j = 0; j < 128; ++j)
    {
      Hand h1 = (Hand)0, h2 = (Hand)0;
      for (int k = 0; k < 7; ++k)
      {
        int bit = (1 << k);
        if (i & bit) add_hand(h1, (Piece)(k + 1), 1);
        if (j & bit) add_hand(h2, (Piece)(k + 1), 1);
      }

      // h1のほうがh2より優れているか。
      bool is_equal_or_superior =
        hand_count(h1, PAWN) >= hand_count(h2, PAWN) &&
        hand_count(h1, LANCE) >= hand_count(h2, LANCE) &&
        hand_count(h1, KNIGHT) >= hand_count(h2, KNIGHT) &&
        hand_count(h1, SILVER) >= hand_count(h2, SILVER) &&
        hand_count(h1, BISHOP) >= hand_count(h2, BISHOP) &&
        hand_count(h1, ROOK) >= hand_count(h2, ROOK) &&
        hand_count(h1, GOLD) >= hand_count(h2, GOLD);

      if (is_equal_or_superior != hand_is_equal_or_superior(h1, h2))
      {
        cout << "error " << h1 << " & " << h2 << endl;
      }
    }

  cout << "Test Hand end." << endl;
}

// 棋譜を丸読みして、棋譜の指し手が生成した合法手のなかに含まれるかをテストする
void test_read_record(Position& pos, istringstream& is)
{
  string filename = "records.sfen";
  is >> filename;
  cout << "read " << filename << endl;

  fstream fs;
  fs.open(filename, ios::in | ios::binary);
  if (fs.fail())
  {
    cout << "read error.." << endl;
    return;
  }

  // ファイル内の行番号
  uint64_t line_no = 0;

  string line;
  while (getline(fs, line))
  {
    ++line_no;
    if ((line_no % 100) == 0) cout << '.'; // 100行おきに'.'を一つ出力。

    stringstream ss(line);
    string token;
    string sfen;

    ss >> token;

    if (token == "startpos")
    {
      // 初期局面として初期局面のFEN形式の入力が与えられたとみなして処理する。
      sfen = SFEN_HIRATE;
      ss >> token; // もしあるなら"moves"トークンを消費する。
    } else if(token == "sfen")
      while (ss >> token && token != "moves")
        sfen += token;

    auto& SetupStates = Search::SetupStates;
    SetupStates = Search::StateStackPtr(new aligned_stack<StateInfo>());

    pos.set(sfen);

    while (ss >> token)
    {
      for (auto m : MoveList<LEGAL_ALL>(pos))
      {
        if (token == to_usi_string(m))
        {
          SetupStates->push(StateInfo());
          pos.do_move(m, SetupStates->top());
          goto Ok;
        }
      }
      // 生成した合法手にない文字列であった。

      cout << "\nError at line number = " << line_no << " : illigal moves = " << token << endl;
      cout << "> " << line << endl; // この行、まるごと表示させておく。
      break;

    Ok:;
    }

  }
  cout << "done." << endl;
}

void auto_play(Position& pos, istringstream& is)
{
  uint64_t loop_max = 50000; // default 5万回
  is >> loop_max;
  cout << "Auto Play test , loop_max = " << loop_max << endl;

  const int MAX_PLY = 256; // 256手までテスト

  StateInfo state[MAX_PLY]; // StateInfoを最大手数分だけ
  Move moves[MAX_PLY]; // 局面の巻き戻し用に指し手を記憶
  int ply; // 初期局面からの手数

  auto start = now();

  Search::LimitsType lm;
  lm.silent = true;
  lm.time[BLACK] = lm.time[WHITE] = 1000;
  lm.byoyomi[BLACK] = lm.byoyomi[WHITE] = 0;
  Options["NetworkDelay"] = string("1900"); // どうせこれで2秒-1.9秒 = 0.1秒の思考となる。

  // isreadyが呼び出されたものとする。
  Search::clear();

  for (uint64_t i = 0; i < loop_max; ++i)
  {
    pos.set_hirate();
    for (ply = 0; ply < MAX_PLY; ++ply)
    {
      MoveList<LEGAL_ALL> mg(pos);
      if (mg.size() == 0)
        break;

      Time.reset();
      Threads.init_for_slave(pos, lm);
      Threads.start_thinking(pos, lm, Search::SetupStates);
      Threads.main()->wait_for_search_finished();
      auto rootMoves = Threads.main()->rootMoves;
      if (rootMoves.size() == 0)
        break;
      Move m = rootMoves.at(0).pv[0]; // 1番目に並び変わっているはず。

      pos.do_move(m, state[ply]);
      moves[ply] = m;
    }
    // 1局ごとに'.'を出力(進んでいることがわかるように)
    cout << ".";
  }
}

void test_timeman()
{
#ifdef USE_TIME_MANAGEMENT

  // Time Managerの動作テストをする。(思考時間の消費量を調整するときに使う)

  auto simulate = [](Search::LimitsType limits)
  {
//    Options["NetworkDelay2"] = "1200";

    int delay = Options["NetworkDelay"];
    int delay2 = Options["NetworkDelay2"];

    // 最小思考時間が1秒設定でもうまく動くかな？
//    Options["MinimumThinkingTime"] = "1000";

    cout << "initial setting "
      << " time = " << limits.time[BLACK]
      << ", byoyomi = " << limits.byoyomi[BLACK]
      << ", inc = " << limits.inc[BLACK]
      << ", NetworkDelay = " << delay
      << ", NetworkDelay2 = " << delay2
      << ", MinimumThinkingTime = " << Options["MinimumThinkingTime"]
      << ", max_game_ply = " << limits.max_game_ply
      << ", rtime = " << limits.rtime
      << endl;

    Timer time;

    int remain = limits.time[BLACK];

    for (int ply = 1; ply <= limits.max_game_ply; ply += 2)
    {
      limits.time[BLACK] = remain;
      time.init(limits, BLACK, ply);
      cout << "ply = " << ply
        << " , minimum = " << time.minimum()
        << " , optimum = " << time.optimum()
        << " , maximum = " << time.maximum()
        ;

      // 4回に1回はtime.minimum()ぶんだけ使ったとみなす。残り3回はtime.optimum()だけ使ったとみなす。
      int used_time = ((ply % 8) == 1) ?  time.minimum() : time.optimum();
      // 1秒未満繰り上げ。ただし、2秒は計測1秒扱い。
      used_time = ((used_time + delay + 999) / 1000) * 1000;
      if (used_time <= 2000)
        used_time = 1000;

      cout << " , used_time = " << used_time;

      remain -= used_time;
      if (remain < 0)
      {
        remain += limits.byoyomi[BLACK];
        if (remain < 0)
        {
          // 思考時間がマイナスになった。
          cout << "\nERROR! TIME OVER!" << endl;
          break;
        }
        // 秒読み状態なので次回の持ち時間はゼロ。
        remain = 0;
      }

      cout << " , remain = " << remain << endl;
      remain += limits.inc[BLACK];
    }
  };

  Search::LimitsType limits;

  limits.max_game_ply = 256;

  // 5分切れ負けのテスト
  limits.time[BLACK] = 5 * 60 * 1000;
  limits.byoyomi[BLACK] = 0;
  simulate(limits);

  // 10分切れ負けのテスト
  limits.time[BLACK] = 10 * 60 * 1000;
  limits.byoyomi[BLACK] = 0;
  simulate(limits);

  // 10分+秒読み10秒のテスト
  limits.time[BLACK] = 10 * 60 * 1000;
  limits.byoyomi[BLACK] = 10 * 1000;
  simulate(limits);

  // 2時間+秒読み60秒のテスト
  limits.time[BLACK] = 2 * 60 * 60 * 1000;
  limits.byoyomi[BLACK] = 60 * 1000;
  simulate(limits);

  // 3秒 + inc 3秒のテスト
  limits.time[BLACK] = 3 * 1000;
  limits.byoyomi[BLACK] = 0;
  limits.inc[BLACK] = 3 * 1000;
  simulate(limits);

  // 10分 + inc 10秒のテスト
  limits.time[BLACK] = 10 * 60 * 1000;
  limits.byoyomi[BLACK] = 0;
  limits.inc[BLACK] = 10 * 1000;
  simulate(limits);

  // 30分 + inc 10秒のテスト
  limits.time[BLACK] = 30 * 60 * 1000;
  limits.byoyomi[BLACK] = 0;
  limits.inc[BLACK] = 10 * 1000;
  simulate(limits);

  /*
  // rtime = 100のテスト
  limits.time[BLACK] = 0;
  limits.byoyomi[BLACK] = 0;
  limits.inc[BLACK] = 0;
  limits.rtime = 100;
  simulate(limits);
  */

#endif
}


// --- "test unit"コマンド

// 単体テスト

// これを実行するときはASSERT_LV5にしておくといいと思う。
void unit_test(Position& pos, istringstream& is)
{
  cout << "UnitTest start" << endl;
  bool success_all = true;
  auto check = [&](bool success) {
    cout << (success ? "..success" : "..failed") << endl;
    success_all &= success;
  };

  // -- 色んな関数
  {
    cout << "> various function ";

    // -- long effect
    bool success = true;
    for (auto pc : Piece())
    {
      auto pt = type_of(pc);
      success &= (pt == LANCE || pt == BISHOP || pt == ROOK || pt == HORSE || pt == DRAGON) == (has_long_effect(pc));
    }
    check(success);
  }

  // -- Mate1Ply
#if defined(MATE_1PLY) && defined(LONG_EFFECT_LIBRARY)
  {
    cout << "> mate1ply check ";

    struct MateProblem
    {
      string sfen;
      Move move;
    };

    // 1手詰め問題集
    MateProblem mp[] = {
      // 影の利きによる移動の詰み
      { "lnsg1gsnl/1r5b1/ppppppppp/5k3/9/5+P3/PPPPP1PPP/1B3R3/LNSGKGSNL b - 1" , make_move(SQ_46,SQ_45)},

      // 駒打ちによる詰み
      { "ln1g1gs1l/1r3p3/pppp1bppp/4pkn+R1/3Ns4/4PP1P1/PPPP1SP1P/5G1B1/L1SGK1L2 b N 1",make_move_drop(KNIGHT,SQ_36) },
      { "ln1g1gs1l/1r3p1b1/pppp2ppp/4pkn2/3Ns4/4PP1+R1/PPPP1SPPP/5G3/LNSGK4 b BL 1",make_move_drop(LANCE,SQ_45) },
      { "ln1g1gs1l/1r5b1/pppp1pppp/4pkn2/3Ns4/4PP1R1/PPPP2PPP/5G3/LNSGK1S1L b B 1",make_move_drop(BISHOP,SQ_53) },
      { "ln1g1gs1l/1r5b1/pppp1pppp/3spkn2/7R1/4PP3/PPPP2PPP/9/LNSGK1SNL b BG 1",make_move_drop(BISHOP,SQ_35) },
      { "1nsg1+B1n+P/lr1k1s3/1pp3p1p/p2ppp3/9/7R1/PPPPPPP1P/1B7/LNSGKGSNL b PLG 21",make_move_drop(GOLD,SQ_63) },
      { "ln1g1gsnl/1r5b1/ppppppppp/4sk3/7R1/9/PPPPPPPPP/1B7/LNSGK1SNL b G 1",MOVE_NONE }, // 詰みじゃない局面
      { "ln1g1gsnl/1r5b1/ppppppppp/4sk3/7R1/4P4/PPPP1PPPP/1B7/LNSGK1SNL b G 1", make_move_drop(GOLD, SQ_35) },

      // 移動による詰み
      { "ln1g1gs1l/1r3p3/pppp1bppp/4pkn+R1/3Ns4/4PP1P1/PPPP1SPBP/5G1N1/L1SGK1L2 b - 1",make_move(SQ_28,SQ_36) },
      { "ln1g1gs1l/1r3p3/pppp1bppp/4pkn+R1/3Ns4/4PP1P1/PPPP1SPNP/5GB2/L1SGK1L2 b - 1",make_move(SQ_24,SQ_35) },
      { "ln1g1gs1l/1r1S1p3/pppp1bppp/4pkn1+R/3Ns4/4PP1P1/PPPP2PNP/5GB2/L1SGK1L2 b - 1",make_move(SQ_62,SQ_53) },
      { "png1+N4/s4P1s1/2p3R1l/2kSppPpp/lP2P1p2/L2P1+bG1P/2N3+p2/gpPR4L/B1KG3+n1 b 2PS 131",MOVE_NONE },
      { "1k5+L1/3r2gPS/2p1p4/gP2PPpN1/G1K1gLs1p/LS1P1R1sP/P1+b3Pn1/1p1p3NL/3+b5 w P3pn 212",MOVE_NONE },
    };

    for (auto& problem : mp)
    {
      Move m;
      pos.set(problem.sfen);
      m = pos.mate1ply();
      bool success = m == problem.move;
      if (!success)
      {
        cout << pos << m << " is wrong." << endl;
        m = pos.mate1ply(); // ここにブレークポイント仕掛けれ。
      }

      success_all &= success;
    }
    check(success_all);

  }
#endif

  // hash key
  // この値が変わると定跡DBがhitしなくなってしまうので変えてはならない。
  {
    cout << "> hash key check ";
    pos.set_hirate();
    check( pos.state()->key() == UINT64_C(0x75a12070b8bd438a));
  }

  // perft
  {
    // 最多合法手局面
    const string POS593 = "R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b RBGSNLP3g3n17p 1";
    cout << "> genmove sfen = " << POS593;
    pos.set(POS593);
    auto mg = MoveList<LEGAL_ALL>(pos);
    cout << " , moves = " << mg.size();
    check( mg.size() == 593);

    cout << "> perft depth 6 ";
    pos.set_hirate();
    auto result = PerftSolver().Perft<true>(pos,6);
    check(  result.nodes == 547581517 && result.captures == 3387051
#ifdef      KEEP_LAST_MOVE
      && result.promotions == 1588324
#endif
      && result.checks == 1730177 && result.mates == 0);
  }

  // random pleyer
  {
    cout << "> random player 100000 ";
    random_player(pos, 100000);

    // ASSERTに引っかからずに抜けたということは成功だという扱い。
    check(true);
  }

  cout << "UnitTest end : " << (success_all ? "success" : "failed") << endl;
}


// 定跡の精査用コマンド
void exam_book(Position& pos)
{
	// やねうら大定跡のスコアと比較して、定跡のうち互角でない局面をそぎ落とし、
	// 自己対戦時の精度を上げるのが狙い。

	// 定跡ファイル
	Book::MemoryBook book;

	string book_name = "yaneura_book1.db";
	Book::read_book("book/" + book_name, book, (bool)Options["BookOnTheFly"]);
	
	string input_sfen_name = "book/records2016.sfen";
	string output_sfen_name = "book/records2016new.sfen";

	cout << "book examine from " << input_sfen_name << " to " << output_sfen_name << endl;
	
	fstream fs1, fs2;
	fs1.open(input_sfen_name, ios::in);
	fs2.open(output_sfen_name, ios::out);

	// 24手目の局面で。
	int moves = 24;

	// 行番号
	int k = 0;
	string line;
	vector<StateInfo> si(moves);

	// 探索済みのsfen(重複局面の除去用)
	std::unordered_set<string> sfens;

	while (!fs1.eof())
	{
		getline(fs1,line);
		++k;
		// 1行読み込んで、評価値を調べる。

		// いま読み込み中の行の読み込んだところまでの内容
		string buf;
		int m = 0; // 0手目から

		stringstream ss(line);
		while (m < moves)
		{
			string token;
			ss >> token;
			buf += token + " ";
			if (token == "startpos")
			{
				pos.set_hirate();
				continue;
			}
			else if (token == "moves")
				continue; // 読み飛ばす

			Move move = move_from_usi(pos, token);
			// illigal moveであるとMOVE_NONEが返る。
			if (move == MOVE_NONE)
			{
				std::cout << "illegal move : line = " << k << " , move = " << token << endl;
				break;
			}
			pos.do_move(move, si[m]);
			++m;
		}

		string sfen = pos.sfen();
		if (sfens.count(sfen) == 0)
		{
			sfens.insert(sfen);

			// この局面で定跡を調べる。
			auto it = book.find(pos);
			if (it != book.end() && it->second.size() != 0)
			{
				int v = it->second[0].value;
				// 得られた評価値が基準範囲内なので定跡として書き出す。
				if (-100 <= v && v <= 100)
				{
					fs2 << buf << endl;
					// within the range
					std::cout << 'O';
				} else {
					// out of range
					std::cout << 'X';
				}
			} else {
				// not found
				std::cout << '.';
			}
		} else {
			// already examined
			std::cout << '_';
		}

	}
	fs1.close();
	fs2.close();
	std::cout << ".. done!" << endl;
}


void book_check(Position& pos, Color rootTurn, Book::MemoryBook& book, string sfen, ofstream& of)
{
	int ply = pos.game_ply();
	StateInfo si;

	auto it = book.find(pos);
	if (it != book.end() && it->second.size() != 0) {
		// 定跡にhitした。逆順で出力しないと将棋所だと逆順にならないという問題があるので逆順で出力する。
		// また、it->second->size()!=0をチェックしておかないと指し手のない定跡が登録されていたときに困る。

		const auto& move_list = it->second;

		// 上位N手で局面を進める。
		size_t n;
		if (pos.side_to_move() == rootTurn)
		{
			// 自分の手番なのでN=1
			n = 1;
		} else {
#if 0
			// 4手目までは4手ずつ候補をあげる。
			if (ply <= 4)
				n = 4;
			else
				n = 2;
#else
			// 常に相手側の平均分岐数は4に設定すればどうか。
			n = 4;
#endif
		}

		for (size_t i = 0; i < n; ++i)
		{
			if (move_list.size() <= i)
				break;

			Move m = move_list[i].bestMove;

			pos.do_move(m, si);
			book_check(pos, rootTurn, book, sfen + ' ' + to_usi_string(m), of);
			pos.undo_move(m);
		}

	} else {
		// 終端になったのでここまでの手順を書き出す。
		of << sfen << endl;
		//		cout << sfen << endl;
	}
}

// 定跡のチェックコマンド
// あとで消すかも。
void book_check_cmd(Position& pos, istringstream& is)
{
	// 初手から定跡をチェックしていき、そこまでの変化をファイルに書き出す。

	// 先手番として
	// 初手、定跡の指し手、上位1通り(自分の手番なので)
	// 後手、定跡の指し手、上位2通り(相手番なので)
	// 3手目、定跡の指し手、上位1通り
	// 4手目、定跡の指し手、上位2通り
	// .. 以下、同様。

	string turn = "all";
	is >> turn;

	cout << "book check start.." << endl;
	cout << "turn = " << turn << endl;;

	string file_name = "book_records.sfen";
	ofstream of(file_name, ios::out);
	pos.set_hirate();

	// とりあえずファイル名は固定でいいや。
	string book_name = "yaneura_book3.db";

	// bookの読み込み。	
	Book::MemoryBook book;
	Book::read_book("book/" + book_name, book, /*BookOnTheFly*/ false);
	string sfen = "startpos moves";

	if (turn == "all")
	{
		for (auto rootTurn : COLOR)
			book_check(pos, rootTurn, book, sfen, of);
	} else if (turn == "black") {
		book_check(pos, BLACK, book, sfen, of);
	} else if (turn == "white") {
		book_check(pos, WHITE, book, sfen, of);
	}

	of.close();
	cout << "..done , write to " << file_name << endl;
}


#ifdef EVAL_KPPT
//
// eval merge
//  KKPT評価関数の合成用
//   実験的に作ったもの。あとで消すかも。
//

struct KKPT_reader
{
	static const int fe_end = 1548;

	typedef std::array<int32_t, 2> ValueKk;
	typedef std::array<int16_t, 2> ValueKpp;
	typedef std::array<int32_t, 2> ValueKkp;

	ValueKk(*kk_)[SQ_NB][SQ_NB];
	ValueKpp(*kpp_)[SQ_NB][fe_end][fe_end];
	ValueKkp(*kkp_)[SQ_NB][SQ_NB][fe_end];

#define KK_BIN "KK_synthesized.bin"
#define KKP_BIN "KKP_synthesized.bin"
#define KPP_BIN "KPP_synthesized.bin"

	KKPT_reader()
	{
		kk_ = (ValueKk(*)[SQ_NB][SQ_NB])new ValueKk[int(SQ_NB)*int(SQ_NB)];
		kpp_ = (ValueKpp(*)[SQ_NB][fe_end][fe_end])new ValueKpp[int(SQ_NB)*int(fe_end)*int(fe_end)];
		kkp_ = (ValueKkp(*)[SQ_NB][SQ_NB][fe_end])new ValueKkp[int(SQ_NB)*int(SQ_NB)*int(fe_end)];
	}

	void read(string dir)
	{
		{
			std::ifstream ifsKK(path_combine(dir, KK_BIN), std::ios::binary);
			if (ifsKK) ifsKK.read(reinterpret_cast<char*>(kk_), sizeof(*kk_));
			else goto Error;

			// KKP
			std::ifstream ifsKKP(path_combine(dir, KKP_BIN), std::ios::binary);
			if (ifsKKP) ifsKKP.read(reinterpret_cast<char*>(kkp_), sizeof(*kkp_));
			else goto Error;

			// KPP
			std::ifstream ifsKPP(path_combine(dir, KPP_BIN), std::ios::binary);
			if (ifsKPP) ifsKPP.read(reinterpret_cast<char*>(kpp_), sizeof(*kpp_));
			else goto Error;

			return;
		}

	Error:;
		cout << "ERROR! : read error." << endl;
	}

	void write(string dir)
	{
		{
			std::ofstream ofsKK(path_combine(dir, KK_BIN), std::ios::binary);
			if (ofsKK) ofsKK.write(reinterpret_cast<char*>(kk_), sizeof(*kk_));
			else goto Error;

			// KKP
			std::ofstream ofsKKP(path_combine(dir, KKP_BIN), std::ios::binary);
			if (ofsKKP) ofsKKP.write(reinterpret_cast<char*>(kkp_), sizeof(*kkp_));
			else goto Error;

			// KPP
			std::ofstream ofsKPP(path_combine(dir, KPP_BIN), std::ios::binary);
			if (ofsKPP) ofsKPP.write(reinterpret_cast<char*>(kpp_), sizeof(*kpp_));
			else goto Error;

			return;
		}

	Error:;
		cout << "ERROR! : write error." << endl;
	}

	// 按分する
	void div(const KKPT_reader& eval2, double percent)
	{
		auto r1 = percent / 100.0;
		auto r2 = 1 - r1;
		// p1:p2で合成する。
		
		for (auto k1 : SQ)
			for (auto k2 : SQ)
			{
				(*kk_)[k1][k2][0] = (s32)(r1 * (*kk_)[k1][k2][0] + r2 * (*eval2.kk_)[k1][k2][0]);
				(*kk_)[k1][k2][1] = (s32)(r1 * (*kk_)[k1][k2][1] + r2 * (*eval2.kk_)[k1][k2][1]);
			}

		for (auto k1 : SQ)
			for (int p1 = 0;p1<fe_end;++p1)
				for (int p2 = 0; p2 < fe_end; ++p2)
				{
					(*kpp_)[k1][p1][p2][0] = (s16)(r1 * (*kpp_)[k1][p1][p2][0] + r2 * (*eval2.kpp_)[k1][p1][p2][0]);
					(*kpp_)[k1][p1][p2][1] = (s16)(r1 * (*kpp_)[k1][p1][p2][1] + r2 * (*eval2.kpp_)[k1][p1][p2][1]);
				}

		for (auto k1 : SQ)
			for (auto k2 : SQ)
				for (int p1 = 0; p1 < fe_end; ++p1)
				{
					(*kkp_)[k1][k2][p1][0] = (s32)(r1 * (*kkp_)[k1][k2][p1][0] + r2 * (*eval2.kkp_)[k1][k2][p1][0]);
					(*kkp_)[k1][k2][p1][1] = (s32)(r1 * (*kkp_)[k1][k2][p1][1] + r2 * (*eval2.kkp_)[k1][k2][p1][1]);
				}
	}

	void normalize_kpp()
	{
		for (int p1 = 0; p1 < fe_end; ++p1)
			for (int p2 = 0; p2 < fe_end; ++p2)
			{
				int sum = 0;
				for (auto sq : SQ)
					sum += (*kpp_)[sq][p1][p2][1];

				int z = sum / SQ_NB;

				for (auto sq : SQ)
					(*kpp_)[sq][p1][p2][1] = z;
			}
	}
};

// "test evalmerge dir1 dir2 dir3 percent"
// dir3は出力フォルダ。事前に生成しておくこと。
void eval_merge(istringstream& is)
{
	string dir1, dir2,dir3;
	double percent;

	// dir1のほうの評価関数を何%で按分するか。
	// 20を指定すると、dir1:dir2 = 20:80で按分する。
	is >> dir1 >> dir2 >> dir3 >> percent;
	cout << "\neval merge KKPT"; // とりあえずKKPT型評価関数のmerge専用。
	cout << "\ndir1    : " << dir1;
	cout << "\ndir2    : " << dir2;
	cout << "\nOutDir  : " << dir3;
	cout << "\npercent : " << percent << endl;

	KKPT_reader eval1, eval2;
	eval1.read(dir1);
	eval2.read(dir2);
	eval1.div(eval2, percent);
//	eval1.normalize_kpp();
	eval1.write(dir3);

	cout << "..done" << endl;
}
#endif

#ifdef EVAL_LEARN
namespace Learner
{
	extern pair<Value, vector<Move> > qsearch(Position& pos);
	extern pair<Value, vector<Move> >  search(Position& pos, int depth);
}

void dump_sfen(Position& pos, istringstream& is)
{
	is_ready();

	std::string filename;
	is >> filename;

	// この番号の局面から。
	u64 start_number = 0;
	u64 end_number = UINT64_MAX;
	while (true)
	{
		std::string token = "";
		is >> token;
		if (token == "start")
			is >> start_number;
		else if (token == "end")
			is >> end_number;
		else
			break;
	}

	cout << "dump sfen , filename = " << filename << endl
		<< "start : " << start_number << endl
		<< "end   : " << end_number << endl;

	fstream fs(filename, ios::in | ios::binary);
	if (fs.fail())
	{
		cout << "Error : file read error " << filename << endl;
		return;
	}

	Learner::PackedSfenValue sfen;

	u64 num = 0;

#if 0
	// 統計用の変数
	u64 sum = 0;
	double vari = 0.0;
#endif

	while (!fs.eof())
	{
		if (!fs.read((char*)&sfen, sizeof(Learner::PackedSfenValue)))
			break;

		// 指定番号になるまでskip
		if (num < start_number)
			goto NEXT;
		
		// 指定番号を超えたら終了。
		if (num >= end_number)
			break;

		pos.set_from_packed_sfen(sfen.sfen);
#if 0
		cout << pos;
		cout << "value = " << sfen.score << " , num = " << num << endl;
#endif

#if 0
		// 評価値の絶対値の平均を計算する。
		sum += abs(sfen.score);

		// 評価値の分散
		vari += sfen.score * sfen.score;

		const u64 block = 10000;
		if ((num % block) == 0)
		{
			// 平均と偏差を出す。
			cout << num << " , avg = " << (sum / block) << " , deviation = " << sqrt(vari/block) << endl;
			sum = 0;
			vari = 0.0;

#if 0
			pos.set_this_thread(Threads.main());
			// 深さ6,8,10,12で探索させた評価値を比較してみる。
			auto pv6 = Learner::search(pos, (Value)-3000, (Value)+3000, 6 * ONE_PLY);
			auto pv8 = Learner::search(pos, (Value)-3000, (Value)+3000, 8 * ONE_PLY);
			auto pv10 = Learner::search(pos, (Value)-3000, (Value)+3000, 10 * ONE_PLY);
			auto pv12 = Learner::search(pos, (Value)-3000, (Value)+3000, 12 * ONE_PLY);
			cout << sfen.score << " and pv6,8 =  " << pv6.first << " , " << pv8.first << " , " << pv10.first << " , " << pv12.first << endl;
#endif
		}
#endif
	NEXT:;
		num++;
	}

	fs.close();

	cout << "sfen_dump , finished." << endl;
}
#endif

void test_cmd(Position& pos, istringstream& is)
{
	is_ready();

	std::string param;
	is >> param;
	if (param == "unit") unit_test(pos, is);                         // 単体テスト
	else if (param == "rp") random_player_cmd(pos, is);              // ランダムプレイヤー
	else if (param == "rpbench") random_player_bench_cmd(pos, is);   // ランダムプレイヤーベンチ
	else if (param == "cm") cooperation_mate_cmd(pos, is);           // 協力詰めルーチン
	else if (param == "checks") test_genchecks(pos, is);             // 王手生成ルーチンのテスト
	else if (param == "hand") test_hand();                           // 手駒の優劣関係などのテスト
	else if (param == "records") test_read_record(pos, is);          // 棋譜の読み込みテスト 
	else if (param == "autoplay") auto_play(pos, is);                // 思考ルーチンを呼び出しての連続自己対戦
	else if (param == "timeman") test_timeman();                     // TimeManagerのテスト
	else if (param == "exambook") exam_book(pos);                    // 定跡の精査用コマンド
	else if (param == "bookcheck") book_check_cmd(pos,is);           // 定跡のチェックコマンド
#ifdef EVAL_LEARN
	else if (param == "dumpsfen") dump_sfen(pos, is);                // gensfenコマンドで生成した教師局面のダンプ
#endif
#ifdef EVAL_KPPT
	else if (param == "evalmerge") eval_merge(is);                   // 評価関数の合成コマンド
#endif
	else {
		cout << "test unit               // UnitTest" << endl;
		cout << "test rp                 // Random Player" << endl;
		cout << "test rpbench            // Random Player bench" << endl;
		cout << "test cm [depth]         // Cooperation Mate" << endl;
		cout << "test checks             // Generate Checks Test" << endl;
		cout << "test records [filename] // Read records.sfen Test" << endl;
		cout << "test autoplay           // Auto Play Test" << endl;
		cout << "test timeman            // Time Manager Test" << endl;
		cout << "test exambook           // Examine Book" << endl;
		cout << "test dumpsfen [filename]// dump gensfen's file" << endl;
	}
}

// ----------------------------------
//  USI拡張コマンド "bench"(ベンチマーク)
// ----------------------------------

// benchmark用デフォルトの局面集
// これを増やすなら、下のほうの fens.assign のところの局面数も増やすこと。
static const char* BenchSfen[] = {

	// 初期局面に近い曲面。
	//"lnsgkgsnl/1r7/p1ppp1bpp/1p3pp2/7P1/2P6/PP1PPPP1P/1B3S1R1/LNSGKG1NL b - 9",
	
	// 読めば読むほど後手悪いような局面
	"l4S2l/4g1gs1/5p1p1/pr2N1pkp/4Gn3/PP3PPPP/2GPP4/1K7/L3r+s2L w BS2N5Pb 1",

	// 57同銀は詰み、みたいな。
	// 読めば読むほど先手が悪いことがわかってくる局面。
	"6n1l/2+S1k4/2lp4p/1np1B2b1/3PP4/1N1S3rP/1P2+pPP+p1/1p1G5/3KG2r1 b GSN2L4Pgs2p 1",

	// 指し手生成祭りの局面
	// cf. http://d.hatena.ne.jp/ak11/20110508/p1
	"l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w RGgsn5p 1",
};

void bench_cmd(Position& pos, istringstream& is)
{
  string token;
  Search::LimitsType limits;
  vector<string> fens;

  // →　デフォルト1024にしておかないと置換表あふれるな。
  string ttSize = (is >> token) ? token : "1024";

  string threads = (is >> token) ? token : "1";
  string limit = (is >> token) ? token : "17";

  string fenFile = (is >> token) ? token : "default";
  string limitType = (is >> token) ? token : "depth";
  
  if (ttSize == "d")
  {
    // デバッグ用の設定(毎回入力するのが面倒なので)
    ttSize = "1024";
    threads = "1";
    fenFile = "default";
    limitType = "depth";
    limit = "6";
  }

  if (limitType == "time")
    limits.movetime = 1000 * stoi(limit); // movetime is in ms

  else if (limitType == "nodes")
    limits.nodes = stoll(limit);

  else if (limitType == "mate")
    limits.mate = stoi(limit);

  else
    limits.depth = stoi(limit);

  Options["Hash"] = ttSize;
  Options["Threads"] = threads;

#if defined(YANEURAOU_2017_EARLY_ENGINE)
  // 定跡にhitされるとベンチマークにならない。
  Options["BookFile"] = "no_book";
#endif

  // ベンチマークモードにしておかないとPVの出力のときに置換表を漁られて探索に影響がある。
  limits.bench = true;

  TT.clear();

  // Optionsの影響を受けると嫌なので、その他の条件を固定しておく。
  limits.enteringKingRule = EKR_NONE;

  // テスト用の局面
  // "default"=デフォルトの局面、"current"=現在の局面、それ以外 = ファイル名とみなしてそのsfenファイルを読み込む
  if (fenFile == "default")
    fens.assign(BenchSfen, BenchSfen + 3);
  else if (fenFile == "current")
    fens.push_back(pos.sfen());
  else
    read_all_lines(fenFile, fens);

  // 評価関数の読み込み等
  is_ready();

  // トータルの探索したノード数
  int64_t nodes = 0;

  // main threadが探索したノード数
  int64_t nodes_main = 0;
  Search::StateStackPtr st;
  
  // ベンチの計測用タイマー
  Timer time;
  time.reset();
  
  for (size_t i = 0; i < fens.size(); ++i)
  {
    Position pos;
    pos.set(fens[i]);
    pos.set_this_thread(Threads.main());
    
    sync_cout << "\nPosition: " << (i + 1) << '/' << fens.size() << sync_endl;

    // 探索時にnpsが表示されるが、それはこのglobalなTimerに基づくので探索ごとにリセットを行なうようにする。
    Time.reset();

    Threads.start_thinking(pos, limits, st);
    Threads.main()->wait_for_search_finished(); // 探索の終了を待つ。

    nodes += Threads.nodes_searched();
    nodes_main += Threads.main()->rootPos.nodes_searched();
  }

  auto elapsed = time.elapsed() + 1; // 0除算の回避のため
  
  sync_cout << "\n==========================="
    << "\nTotal time (ms) : " << elapsed
    << "\nNodes searched  : " << nodes
    << "\nNodes/second    : " << 1000 * nodes / elapsed;

  if ((int)Options["Threads"] > 1)
    cout
    << "\nNodes searched(main thread) : " << nodes_main
    << "\nNodes/second  (main thread) : " << 1000 * nodes_main / elapsed;

  cout << sync_endl;

}

#ifdef MATE_ENGINE
// ----------------------------------
//  USI拡張コマンド "test_mate_engine"
// ----------------------------------

// 詰将棋エンジンテスト用局面集
static const char* TestMateEngineSfen[] = {
  // http://www.ne.jp/asahi/tetsu/toybox/shogi/kifu.htm
  "3sks3/9/4+P4/9/9/+B8/9/9/9 b S2rb4gs4n4l17p 1",
  // http://www.ne.jp/asahi/tetsu/toybox/shogi/kifu.htm
  "7nl/7k1/6p2/6S1p/9/9/9/9/9 b GS2r2b3g2s3n3l16p 1",
  // http://www.ne.jp/asahi/tetsu/toybox/shogi/kifu.htm
  "4k4/9/PPPPPPPPP/9/9/9/9/9/9 b B4L2rb4g4s4n9p 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2Bukamuse_6700K%2Bcatshogi%2B20170430143005.csa&move_to=102
  "l2g5/2s3g2/3k1p2p/P2pp2P1/1pP4s1/p1+B6/NP1P+nPS1P/K1G4+p1/L6NL b RBGNLPrs3p 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2Bcoduck_pi2_600MHz_1c%2BShogiNet%2B20170430110007.csa&move_to=100
  "6lnk/6+Rbl/2n4pp/7s1/1p2P2NP/p1P2PPP1/1P4GS1/6GK1/LNr5L b B2G2S6Pp 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2BSM_1_25_Xeon_E5_2698_v4_40c%2BSILENT_MAJORITY_1.25_6950X%2B20170430103005.csa&move_to=195
  "lnks5/1pg1s4/2p5p/p4+r3/P1g6/1Nn6/BKN1P3P/9/LG2s4 w GSL2Prbl9p 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2Bcatshogi%2Bgps_l%2B20170430070003.csa&move_to=134
  "l7l/2+Rbk4/3rp4/2p3pPs/p2P1p2p/2P1G4/P1N1PPN2/2GK2G2/L7L b B2S6Pgs2n 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2Bcatshogi%2BGikouAperyEvalMix_SeoTsume_i5-33%2B20170430063002.csa&move_to=127
  "l5g1l/2s+B5/p2ppp2p/5kpP1/3n5/6Pp1/P3PP1lP/2+nr2SS1/3N1GKRL w G2Pbgsn3p 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2BGc_at_Cortex-A53_4c%2BSaturday_Crush_4770K%2B20170430023007.csa&move_to=99
  "l4g2l/7k1/p1+Pp3pp/5ss1P/3Pp1gP1/P3SL3/N2GPK3/1+rP6/+p6RL w BG2N2Pbsn3p 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2BSM_1_25_Xeon_E5_2698_v4_40c%2Bukamuse_i7%2B20170430013007.csa&move_to=116
  "l2s3nl/3g1p+R+R1/p1k5p/2pPp4/1p1p5/5Sp2/PPP1PP2P/3G5/L1K4NL b BG2S2Pbg2np 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2BGc_at_Cortex-A53_4c%2Bsonic%2B20170430013003.csa&move_to=149
  "ln7/2gk1S+S2/2+rpPp2G/2p5p/PP4P2/3B4P/K1SP3PN/1Sg2P+np1/L+r6L w L2Pbgn3p 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2BTest_NB10.5_i5_6200U%2BGikouAperyEvalMix_SeoTsume_i5-33%2B20170430010007.csa&move_to=121
  "6p1l/1+R1G2g2/5pns1/pp1pk3p/2p3P2/P7P/1L1PSP+b2/1SG1K2P1/L5G1L w N2Prbs2n3p 1",
  // http://wdoor.c.u-tokyo.ac.jp/shogi/view/index.cgi?csa=http://wdoor.c.u-tokyo.ac.jp/shogi/LATEST/2017/04/30/wdoor%2Bfloodgate-300-10F%2BInoue%2Byeu%2B20170430003006.csa&move_to=144
  "lng3+R2/2kgs4/ppp6/1B1pp4/7B1/2P2pLp1/PP1PP3P/1S1K2p2/LN5GL b RG2SP2n3p 1",
};

void test_mate_engine_cmd(Position& pos, istringstream& is) {
  string token;

  // →　デフォルト1024にしておかないと置換表あふれるな。
  string ttSize = (is >> token) ? token : "1024";

  Options["Hash"] = ttSize;

  Search::LimitsType limits;

  // ベンチマークモードにしておかないとPVの出力のときに置換表を漁られて探索に影響がある。
  limits.bench = true;

  // Optionsの影響を受けると嫌なので、その他の条件を固定しておく。
  limits.enteringKingRule = EKR_NONE;

  // 評価関数の読み込み等
  is_ready();

  // トータルの探索したノード数
  int64_t nodes = 0;

  // main threadが探索したノード数
  int64_t nodes_main = 0;
  Search::StateStackPtr st;

  // ベンチの計測用タイマー
  Timer time;
  time.reset();

  for (const char* sfen : TestMateEngineSfen) {
    Position pos;
    pos.set(sfen);
    pos.set_this_thread(Threads.main());

    sync_cout << "\nPosition: " << sfen << sync_endl;

    // 探索時にnpsが表示されるが、それはこのglobalなTimerに基づくので探索ごとにリセットを行なうようにする。
    Time.reset();

    Threads.start_thinking(pos, limits, st);
    Threads.main()->wait_for_search_finished(); // 探索の終了を待つ。

    nodes += Threads.nodes_searched();
    nodes_main += Threads.main()->rootPos.nodes_searched();
  }

  auto elapsed = time.elapsed() + 1; // 0除算の回避のため

  sync_cout << "\n==========================="
    << "\nTotal time (ms) : " << elapsed
    << "\nNodes searched  : " << nodes
    << "\nNodes/second    : " << 1000 * nodes / elapsed;

  if ((int)Options["Threads"] > 1)
    cout
    << "\nNodes searched(main thread) : " << nodes_main
    << "\nNodes/second  (main thread) : " << 1000 * nodes_main / elapsed;

  cout << sync_endl;

}
#endif

#endif // ENABLE_TEST_CMD
