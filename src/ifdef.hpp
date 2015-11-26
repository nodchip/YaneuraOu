﻿#ifndef APERY_IFDEF_HPP
#define APERY_IFDEF_HPP

#if 0
// 機械学習を行う際に使う。
#define LEARN
#if 0
// MPI で複数台のPCを使って学習する。
// todo: 実装はまだ。
#define MPI_LEARN
#endif
#endif

#define EVAL_PHASE1
#define EVAL_PHASE2
#define EVAL_PHASE3
#define EVAL_PHASE4

#if 1 && !defined LEARN
// 対局時は1つの局面に対してしか探索を実行しないので、置換表などのデータをグローバルに置いて高速化する。
// 機械学習を行う時は、複数の局面に対して同時に探索を実行する為に、
// クラスで持つようにする。
#define USE_GLOBAL
#define STATIC static
#else
#define STATIC
#endif

#if 1
// 評価関数の SIMD 化
#if defined HAVE_AVX2
#define USE_AVX2_EVAL
#elif defined HAVE_SSE4
#define USE_SSE_EVAL
#endif
#endif

#if 0
// 玉の位置にボーナスを与える。
// 入玉を狙ったり、相手の入玉を阻止したりする為に使う。
// 評価関数は普段はこれをoffにした状態で学習する。
// 有効にしたなら追加である程度学習して馴染ませる必要がある。
#define USE_K_FIX_OFFSET
#endif

#if 0
// 定跡作成時に探索を用いて定跡に点数を付ける。
#define MAKE_SEARCHED_BOOK
#endif

#if 0
// 対局で使わない機能を全て省いたものにする。
// todo: 現状メンテナンスされていないのでやること。
#define MINIMUL
#endif

#if 0
// 稲庭判定、稲庭対策を有効にする。
#define INANIWA_SHIFT
#endif

#if 0
// △２八角、△７八角 を打たないように点数を補正する。
#define BISHOP_IN_DANGER
#endif

#if 0
// 入玉を24点法にする。
#define LAW_24
#endif

#if 0
// 探索時に片方だけが千日手を禁止して考える。
#define BAN_BLACK_REPETITION
#elif 0
#define BAN_WHITE_REPETITION
#endif

#if 0
// Magic Bitboard で必要となるマジックナンバーを求める。
#define FIND_MAGIC
#endif

#if 1
// mask gather を使用する
#define USE_MASK_GATHER
#endif

#if 0
#define RECORD_ITERATIVE_DEEPNING_SCORES
#endif

#if 0
// Aspiration Windows Search で次のイテレーションの値を予測する
#define USE_ASPIRATION_WINDOW_PREDICTION
#endif

#if 0
// 置換表がどれくらい埋まっているか出力する
#define OUTPUT_TRANSPOSITION_TABLE_UTILIZATION
#elif 0
// 評価値キャッシュがどれくらい埋まっているか出力する
#define OUTPUT_EVALUATE_HASH_TABLE_UTILIZATION
#elif 0
// 置換表のヒット率を出力する
#define OUTPUT_TRANSPOSITION_HIT_RATE
#elif 0
// 評価値キャッシュのヒット率を出力する
#define OUTPUT_EVALUATE_HASH_HIT_RATE
#elif 0
// 置換表の破棄率を出力する
#define OUTPUT_TRANSPOSITION_EXPIRATION_RATE
#elif 0
// 評価値キャッシュの破棄率を出力する
#define OUTPUT_EVALUATE_HASH_EXPIRATION_RATE
#endif

#endif // #ifndef APERY_IFDEF_HPP
