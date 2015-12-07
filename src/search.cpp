#include "search.hpp"
#include "position.hpp"
#include "usi.hpp"
#include "evaluate.hpp"
#include "movePicker.hpp"
#include "tt.hpp"
#include "generateMoves.hpp"
#include "thread.hpp"
#include "timeManager.hpp"
#include "book.hpp"

// 一箇所でしか呼ばないので、FORCE_INLINE
FORCE_INLINE void ThreadPool::wakeUp(Searcher* s) {
  for (size_t i = 0; i < size(); ++i) {
    (*this)[i]->maxPly = 0;
  }
  sleepWhileIdle_ = s->options[OptionNames::USE_SLEEPING_THREADS];
}
// 一箇所でしか呼ばないので、FORCE_INLINE
FORCE_INLINE void ThreadPool::sleep() {
  sleepWhileIdle_ = true;
}

#if defined USE_GLOBAL
SignalsType Searcher::signals;
LimitsType Searcher::limits;
std::vector<Move> Searcher::searchMoves;
Time Searcher::searchTimer;
u64 Searcher::lastSearchedNodes;
StateStackPtr Searcher::setUpStates;
std::vector<RootMove> Searcher::rootMoves;
size_t Searcher::pvSize;
size_t Searcher::pvIdx;
std::unique_ptr<TimeManager> Searcher::timeManager;
Ply Searcher::bestMoveChanges;
History Searcher::history;
Gains Searcher::gains;
TranspositionTable Searcher::tt;
#if defined INANIWA_SHIFT
InaniwaFlag Searcher::inaniwaFlag;
#endif
#if defined BISHOP_IN_DANGER
BishopInDangerFlag Searcher::bishopInDangerFlag;
#endif
Position Searcher::rootPosition(nullptr);
ThreadPool Searcher::threads;
OptionsMap Searcher::options;
Searcher* Searcher::thisptr;
Book Searcher::book;
#endif
bool Searcher::outputInfo = true;

void Searcher::init() {
#if defined USE_GLOBAL
#else
  thisptr = this;
#endif
  options.init(thisptr);
  threads.init(thisptr);
  tt.resize(options[OptionNames::USI_HASH]);
}

namespace {
  // info を標準出力へ出力するスロットル
  // 前回出力してから以下の時間を経過していない場合は出力しない
  static constexpr int THROTTLE_TO_OUTPUT_INFO_MS = 200;
  static constexpr Score INITIAL_ASPIRATION_WINDOW_WIDTH = (Score)16;
  static constexpr Score SECOND_ASPIRATION_WINDOW_WIDTH = (Score)64;
  // true にすると、シングルスレッドで動作する。デバッグ用。
  constexpr bool FakeSplit = false;

  inline Score razorMargin(const Depth d) {
    return static_cast<Score>(512 + 16 * static_cast<int>(d));
  }

  Score FutilityMargins[16][64]; // [depth][moveCount]
  inline Score futilityMargin(const Depth depth, const int moveCount) {
    return (depth < 7 * OnePly ?
      FutilityMargins[std::max(depth, Depth1)][std::min(moveCount, 63)]
      : 2 * ScoreInfinite);
  }

  int FutilityMoveCounts[32];    // [depth]

  s8 Reductions[2][64][64]; // [pv][depth][moveNumber]
  template <bool PVNode> inline Depth reduction(const Depth depth, const int moveCount) {
    return static_cast<Depth>(Reductions[PVNode][std::min(Depth(depth / OnePly), Depth(63))][std::min(moveCount, 63)]);
  }

  struct Skill {
    Skill(const int l, const int mr)
      : level(l),
      max_random_score_diff(static_cast<Score>(mr)),
      best(Move::moveNone()) {}
    ~Skill() {}
    void swapIfEnabled(Searcher* s) {
      if (enabled()) {
        auto it = std::find(s->rootMoves.begin(),
          s->rootMoves.end(),
          (!best.isNone() ? best : pickMove(s)));
        if (s->rootMoves.begin() != it)
          SYNCCOUT << "info string swap multipv 1, " << it - s->rootMoves.begin() + 1 << SYNCENDL;
        std::swap(s->rootMoves[0], *it);
      }
    }
    bool enabled() const { return level < 20 || max_random_score_diff != ScoreZero; }
    bool timeToPick(const int depth) const { return depth == 1 + level; }
    Move pickMove(Searcher* s) {
      // level については未対応。max_random_score_diff についてのみ対応する。
      if (max_random_score_diff != ScoreZero) {
        size_t i = 1;
        for (; i < s->pvSize; ++i) {
          if (max_random_score_diff < s->rootMoves[0].score_ - s->rootMoves[i].score_)
            break;
        }
        // 0 から i-1 までの間でランダムに選ぶ。
        std::uniform_int_distribution<size_t> dist(0, i - 1);
        best = s->rootMoves[dist(g_randomTimeSeed)].pv_[0];
        return best;
      }
      best = s->rootMoves[0].pv_[0];
      return best;
    }

    int level;
    Score max_random_score_diff;
    Move best;
  };

  inline bool checkIsDangerous() {
    // not implement
    // 使用しないで良いかも知れない。
    return false;
  }

  // 1 ply前の first move によって second move が合法手にするか。
  bool allows(const Position& pos, const Move first, const Move second) {
    const Square m1to = first.to();
    const Square m1from = first.from();
    const Square m2from = second.from();
    const Square m2to = second.to();
    if (m1to == m2from || m2to == m1from) {
      return true;
    }

    if (second.isDrop() && first.isDrop()) {
      return false;
    }

    if (!second.isDrop() && !first.isDrop()) {
      if (betweenBB(m2from, m2to).isSet(m1from)) {
        return true;
      }
    }

    const PieceType m1pt = first.pieceTypeFromOrDropped();
    const Color us = pos.turn();
    const Bitboard occ = (second.isDrop() ? pos.occupiedBB() : pos.occupiedBB() ^ setMaskBB(m2from));
    const Bitboard m1att = pos.attacksFrom(m1pt, us, m1to, occ);
    if (m1att.isSet(m2to)) {
      return true;
    }

    if (m1att.isSet(pos.kingSquare(us))) {
      return true;
    }

    return false;
  }

  Score scoreToTT(const Score s, const Ply ply) {
    assert(s != ScoreNone);

    return (ScoreMateInMaxPly <= s ? s + static_cast<Score>(ply)
      : s <= ScoreMatedInMaxPly ? s - static_cast<Score>(ply)
      : s);
  }

  Score scoreFromTT(const Score s, const Ply ply) {
    return (s == ScoreNone ? ScoreNone
      : ScoreMateInMaxPly <= s ? s - static_cast<Score>(ply)
      : s <= ScoreMatedInMaxPly ? s + static_cast<Score>(ply)
      : s);
  }

  // fitst move によって、first move の相手側の second move を違法手にするか。
  bool refutes(const Position& pos, const Move first, const Move second) {
    assert(pos.isOK());

    const Square m2to = second.to();
    const Square m1from = first.from(); // 駒打でも今回はこれで良い。

    if (m1from == m2to) {
      return true;
    }

    const PieceType m2ptFrom = second.pieceTypeFrom();
    if (second.isCaptureOrPromotion()
      && ((pos.pieceScore(second.cap()) <= pos.pieceScore(m2ptFrom))
        || m2ptFrom == King))
    {
      // first により、新たに m2to に当たりになる駒があるなら true
      assert(!second.isDrop());

      const Color us = pos.turn();
      const Square m1to = first.to();
      const Square m2from = second.from();
      Bitboard occ = pos.occupiedBB() ^ setMaskBB(m2from) ^ setMaskBB(m1to);
      PieceType m1ptTo;

      if (first.isDrop()) {
        m1ptTo = first.pieceTypeDropped();
      }
      else {
        m1ptTo = first.pieceTypeTo();
        occ ^= setMaskBB(m1from);
      }

      if (pos.attacksFrom(m1ptTo, us, m1to, occ).isSet(m2to)) {
        return true;
      }

      const Color them = oppositeColor(us);
      // first で動いた後、sq へ当たりになっている遠隔駒
      const Bitboard xray =
        (pos.attacksFrom<Lance>(them, m2to, occ) & pos.bbOf(Lance, us))
        | (pos.attacksFrom<Rook  >(m2to, occ) & pos.bbOf(Rook, Dragon, us))
        | (pos.attacksFrom<Bishop>(m2to, occ) & pos.bbOf(Bishop, Horse, us));

      // sq へ当たりになっている駒のうち、first で動くことによって新たに当たりになったものがあるなら true
      if (xray.isNot0() && (xray ^ (xray & queenAttack(m2to, pos.occupiedBB()))).isNot0()) {
        return true;
      }
    }

    if (!second.isDrop()
      && isSlider(m2ptFrom)
      && betweenBB(second.from(), m2to).isSet(first.to())
      && ScoreZero <= pos.seeSign(first))
    {
      return true;
    }

    return false;
  }

  std::string scoreToUSI(const Score score, const Score alpha, const Score beta) {
    std::stringstream ss;

    int normalizedScore = score * 100 / PawnScore;
    int normalizedAlpha = alpha * 100 / PawnScore;
    int normalizedBeta = beta * 100 / PawnScore;

    if (abs(score) < ScoreMateInMaxPly) {
      // cp は centi pawn の略
      ss << "cp " << normalizedScore;
    }
    else {
      // mate の後には、何手で詰むかを表示する。
      ss << "mate " << (0 < score ? ScoreMate0Ply - score : -ScoreMate0Ply - score);
    }

    ss << (score >= beta ? "^" : score <= alpha ? "v" : "");

    return ss.str();
  }

  inline std::string scoreToUSI(const Score score) {
    return scoreToUSI(score, -ScoreInfinite, ScoreInfinite);
  }
}

std::string Searcher::pvInfoToUSI(Position& pos, const Ply depth, const Score alpha, const Score beta) {
  const int t = searchTimer.elapsed();
  const size_t usiPVSize = pvSize;
  Ply selDepth = 0; // 選択的に読んでいる部分の探索深さ。
  std::stringstream ss;

  for (size_t i = 0; i < threads.size(); ++i) {
    if (selDepth < threads[i]->maxPly) {
      selDepth = threads[i]->maxPly;
    }
  }

  for (size_t i = usiPVSize - 1; 0 <= static_cast<int>(i); --i) {
    const bool update = (i <= pvIdx);

    if (depth == 1 && !update) {
      continue;
    }

    const Ply d = (update ? depth : depth - 1);
    const Score s = (update ? rootMoves[i].score_ : rootMoves[i].prevScore_);

    ss << "info depth " << d
      << " seldepth " << selDepth
      << " score " << (i == pvIdx ? scoreToUSI(s, alpha, beta) : scoreToUSI(s))
      << " nodes " << pos.nodesSearched()
      << " nps " << (0 < t ? pos.nodesSearched() * 1000 / t : 0)
      << " time " << t
      << " multipv " << i + 1
#if defined(OUTPUT_TRANSPOSITION_TABLE_UTILIZATION)
      << " hashfull " << tt.getUtilizationPerMill()
#elif defined(OUTPUT_EVALUATE_HASH_TABLE_UTILIZATION)
      << " hashfull " << g_evalTable.getUtilizationPerMill()
#elif defined(OUTPUT_TRANSPOSITION_HIT_RATE)
      << " hashfull " << tt.getHitRatePerMill()
#elif defined(OUTPUT_EVALUATE_HASH_HIT_RATE)
      << " hashfull " << Evaluater::getHitRatePerMill()
#elif defined(OUTPUT_TRANSPOSITION_EXPIRATION_RATE)
      << " hashfull " << tt.getCacheExpirationRatePerMill()
#elif defined(OUTPUT_EVALUATE_HASH_EXPIRATION_RATE)
      << " hashfull " << Evaluater::getExpirationRatePerMill()
#endif
      << " pv ";

    for (int j = 0; !rootMoves[i].pv_[j].isNone(); ++j) {
      ss << " " << rootMoves[i].pv_[j].toUSI();
    }

    if (i) {
      ss << std::endl;
    }
  }

#ifdef OUTPUT_TRANSPOSITION_EXPIRATION_RATE
  ss << std::endl
    << "info string numSaves=" << tt.getNumberOfSaves()
    << " numExpirations=" << tt.getNumberOfCacheExpirations() << std::endl;
#endif
  return ss.str();
}

template <NodeType NT, bool INCHECK>
Score Searcher::qsearch(Position& pos, SearchStack* ss, Score alpha, Score beta, const Depth depth) {
  constexpr bool PVNode = (NT == PV);

  assert(NT == PV || NT == NonPV);
  assert(INCHECK == pos.inCheck());
  assert(-ScoreInfinite <= alpha && alpha < beta && beta <= ScoreInfinite);
  assert(PVNode || (alpha == beta - 1));
  assert(depth <= Depth0);

  StateInfo st;
  TTEntry* tte;
  Key posKey;
  Move ttMove;
  Move move;
  Move bestMove;
  Score bestScore;
  Score score;
  Score ttScore;
  Score futilityScore;
  Score futilityBase;
  Score oldAlpha;
  bool givesCheck;
  bool evasionPrunable;
  Depth ttDepth;
  bool ttHit;

  if (PVNode) {
    oldAlpha = alpha;
  }

  ss->currentMove = bestMove = Move::moveNone();
  ss->ply = (ss - 1)->ply + 1;

  if (MaxPly < ss->ply) {
    return ScoreDraw;
  }

  ttDepth = ((INCHECK || DepthQChecks <= depth) ? DepthQChecks : DepthQNoChecks);

  posKey = pos.getKey();
  tte = tt.probe(posKey, ttHit);
  ttMove = (ttHit ? move16toMove(tte->move(), pos) : Move::moveNone());
  ttScore = (ttHit ? scoreFromTT(tte->score(), ss->ply) : ScoreNone);

  if (ttHit
    && ttDepth <= tte->depth()
    && ttScore != ScoreNone // アクセス競合が起きたときのみ、ここに引っかかる。
    && (PVNode ? tte->bound() == BoundExact
      : (beta <= ttScore ? (tte->bound() & BoundLower)
        : (tte->bound() & BoundUpper))))
  {
    ss->currentMove = ttMove;
    return ttScore;
  }

  pos.setNodesSearched(pos.nodesSearched() + 1);

  if (INCHECK) {
    ss->staticEval = ScoreNone;
    bestScore = futilityBase = -ScoreInfinite;
  }
  else {
    if (!(move = pos.mateMoveIn1Ply()).isNone()) {
      return mateIn(ss->ply);
    }

    if (ttHit) {
      if ((ss->staticEval = bestScore = tte->eval()) == ScoreNone) {
        ss->staticEval = bestScore = evaluate(pos, ss);
      }
    }
    else {
      ss->staticEval = bestScore = evaluate(pos, ss);
    }

    if (beta <= bestScore) {
      if (tte == nullptr) {
        tte->save(pos.getKey(), scoreToTT(bestScore, ss->ply), BoundLower,
          DepthNone, Move::moveNone(), ss->staticEval, tt.generation());
      }

      return bestScore;
    }

    if (PVNode && alpha < bestScore) {
      alpha = bestScore;
    }

    futilityBase = bestScore + 128; // todo: 128 より大きくて良いと思う。
  }

  evaluate(pos, ss);

  MovePicker mp(pos, ttMove, depth, history, (ss - 1)->currentMove.to());
  const CheckInfo ci(pos);

  while (!(move = mp.nextMove<false>()).isNone())
  {
    assert(pos.isOK());

    givesCheck = pos.moveGivesCheck(move, ci);

    // futility pruning
    if (!PVNode
      && !INCHECK // 駒打ちは王手回避のみなので、ここで弾かれる。
      && !givesCheck
      && move != ttMove)
    {
      futilityScore =
        futilityBase + Position::capturePieceScore(pos.piece(move.to()));
      if (move.isPromotion()) {
        futilityScore += Position::promotePieceScore(move.pieceTypeFrom());
      }

      if (futilityScore < beta) {
        bestScore = std::max(bestScore, futilityScore);
        continue;
      }

      // todo: MovePicker のオーダリングで SEE してるので、ここで SEE するの勿体無い。
      if (futilityBase < beta
        && depth < Depth0
        && pos.see(move, beta - futilityBase) <= ScoreZero)
      {
        bestScore = std::max(bestScore, futilityBase);
        continue;
      }
    }

    evasionPrunable = (INCHECK
      && ScoreMatedInMaxPly < bestScore
      && !move.isCaptureOrPawnPromotion());

    if (!PVNode
      && (!INCHECK || evasionPrunable)
      && move != ttMove
      && (!move.isPromotion() || move.pieceTypeFrom() != Pawn)
      && pos.seeSign(move) < 0)
    {
      continue;
    }

    if (!pos.pseudoLegalMoveIsLegal<false, false>(move, ci.pinned)) {
      continue;
    }

    ss->currentMove = move;

    pos.doMove(move, st, ci, givesCheck);
    (ss + 1)->staticEvalRaw.p[0][0] = ScoreNotEvaluated;
    score = (givesCheck ? -qsearch<NT, true>(pos, ss + 1, -beta, -alpha, depth - OnePly)
      : -qsearch<NT, false>(pos, ss + 1, -beta, -alpha, depth - OnePly));
    pos.undoMove(move);

    assert(-ScoreInfinite < score && score < ScoreInfinite);

    if (bestScore < score) {
      bestScore = score;

      if (alpha < score) {
        if (PVNode && score < beta) {
          alpha = score;
          bestMove = move;
        }
        else {
          // fail high
          tte->save(posKey, scoreToTT(score, ss->ply), BoundLower,
            ttDepth, move, ss->staticEval, tt.generation());
          return score;
        }
      }
    }
  }

  if (INCHECK && bestScore == -ScoreInfinite) {
    return matedIn(ss->ply);
  }

  tte->save(posKey, scoreToTT(bestScore, ss->ply),
    ((PVNode && oldAlpha < bestScore) ? BoundExact : BoundUpper),
    ttDepth, bestMove, ss->staticEval, tt.generation());

  assert(-ScoreInfinite < bestScore && bestScore < ScoreInfinite);

  return bestScore;
}

// iterative deepening loop
void Searcher::idLoop(Position& pos) {
#ifdef RECORD_ITERATIVE_DEEPNING_SCORES
  int scores[MaxPlyPlus2];
#endif

  SearchStack ss[MaxPlyPlus2];
  Ply depth;
  Ply prevBestMoveChanges;
  Score bestScore = -ScoreInfinite;
  Score delta = -ScoreInfinite;
  Score alpha = -ScoreInfinite;
  Score beta = ScoreInfinite;
  bool bestMoveNeverChanged = true;
  // 最後にinfoを出力した時間
  // 将棋所のコンソールが詰まる問題への対処用
  int lastTimeToOutputInfoMs = -1;
  int mateCount = 0;

  memset(ss, 0, 4 * sizeof(SearchStack));
  bestMoveChanges = 0;
#if defined LEARN
  // 高速化の為に浅い探索は反復深化しないようにする。学習時は浅い探索をひたすら繰り返す為。
  depth = std::max<Ply>(0, limits.depth - 1);
#else
  depth = 0;
#endif

  ss[0].currentMove = Move::moveNull(); // skip update gains
  tt.new_search();
  history.clear();
  gains.clear();

  pvSize = options[OptionNames::MULTIPV];
  Skill skill(options[OptionNames::SKILL_LEVEL], options[OptionNames::MAX_RANDOM_SCORE_DIFF]);

  if (options[OptionNames::MAX_RANDOM_SCORE_DIFF_PLY] < pos.gamePly()) {
    skill.max_random_score_diff = ScoreZero;
    pvSize = 1;
    assert(!skill.enabled()); // level による設定が出来るようになるまでは、これで良い。
  }

  if (skill.enabled() && pvSize < 3) {
    pvSize = 3;
  }
  pvSize = std::min(pvSize, rootMoves.size());

  // 指し手が無ければ負け
  if (rootMoves.empty()) {
    rootMoves.push_back(RootMove(Move::moveNone()));
    SYNCCOUT << "info depth 0 score "
      << scoreToUSI(-ScoreMate0Ply)
      << SYNCENDL;

    return;
  }

#if defined BISHOP_IN_DANGER
  if ((bishopInDangerFlag == BlackBishopInDangerIn28
    && std::find_if(std::begin(rootMoves), std::end(rootMoves),
      [](const RootMove& rm) { return rm.pv_[0].toCSA() == "0082KA"; }) != std::end(rootMoves))
    || (bishopInDangerFlag == WhiteBishopInDangerIn28
      && std::find_if(std::begin(rootMoves), std::end(rootMoves),
        [](const RootMove& rm) { return rm.pv_[0].toCSA() == "0028KA"; }) != std::end(rootMoves))
    || (bishopInDangerFlag == BlackBishopInDangerIn78
      && std::find_if(std::begin(rootMoves), std::end(rootMoves),
        [](const RootMove& rm) { return rm.pv_[0].toCSA() == "0032KA"; }) != std::end(rootMoves))
    || (bishopInDangerFlag == WhiteBishopInDangerIn78
      && std::find_if(std::begin(rootMoves), std::end(rootMoves),
        [](const RootMove& rm) { return rm.pv_[0].toCSA() == "0078KA"; }) != std::end(rootMoves)))
  {
    if (rootMoves.size() != 1)
      pvSize = std::max<size_t>(pvSize, 2);
  }
#endif

  // 反復深化で探索を行う。
  while (++depth <= MaxPly && !signals.stop && (!limits.depth || depth <= limits.depth)) {
    // 前回の iteration の結果を全てコピー
    for (size_t i = 0; i < rootMoves.size(); ++i) {
      rootMoves[i].prevScore_ = rootMoves[i].score_;
    }

    prevBestMoveChanges = bestMoveChanges;
    bestMoveChanges = 0;

    // Multi PV loop
    for (pvIdx = 0; pvIdx < pvSize && !signals.stop; ++pvIdx) {
#if defined LEARN
      alpha = this->alpha;
      beta = this->beta;
#else
      // aspiration search
      // alpha, beta をある程度絞ることで、探索効率を上げる。
      if (5 <= depth && abs(rootMoves[pvIdx].prevScore_) < ScoreKnownWin) {
        delta = INITIAL_ASPIRATION_WINDOW_WIDTH;
        alpha = rootMoves[pvIdx].prevScore_ - delta;
        beta = rootMoves[pvIdx].prevScore_ + delta;
      }
      else {
        alpha = -ScoreInfinite;
        beta = ScoreInfinite;
      }
#endif

      // aspiration search の window 幅を、初めは小さい値にして探索し、
      // fail high/low になったなら、今度は window 幅を広げて、再探索を行う。
      while (true) {
        // 探索を行う。
        ss->staticEvalRaw.p[0][0] = (ss + 1)->staticEvalRaw.p[0][0] = ScoreNotEvaluated;
        bestScore = search<Root>(pos, ss + 1, alpha, beta, static_cast<Depth>(depth * OnePly), false);
        // 先頭が最善手になるようにソート
        insertionSort(rootMoves.begin() + pvIdx, rootMoves.end());

        for (size_t i = 0; i <= pvIdx; ++i) {
          ss->staticEvalRaw.p[0][0] = (ss + 1)->staticEvalRaw.p[0][0] = ScoreNotEvaluated;
          rootMoves[i].insertPvInTT(pos);
        }

#if 0
        // 詰みを発見したら即指す。
        if (ScoreMateInMaxPly <= abs(bestScore) && abs(bestScore) < ScoreInfinite) {
          SYNCCOUT << pvInfoToUSI(pos, ply, alpha, beta) << SYNCENDL;
          signals.stop = true;
        }
#endif

#if defined LEARN
        break;
#endif

        if (lastTimeToOutputInfoMs + THROTTLE_TO_OUTPUT_INFO_MS < searchTimer.elapsed()) {
          if (outputInfo) {
            SYNCCOUT << pvInfoToUSI(pos, depth, alpha, beta) << SYNCENDL;
          }
          lastTimeToOutputInfoMs = searchTimer.elapsed();
        }

        if (signals.stop) {
          break;
        }

        if (alpha < bestScore && bestScore < beta) {
          break;
        }

        if (delta == INITIAL_ASPIRATION_WINDOW_WIDTH) {
          delta = SECOND_ASPIRATION_WINDOW_WIDTH;
        }

        // fail high/low のとき、aspiration window を広げる。
        if (ScoreKnownWin <= abs(bestScore)) {
          // 勝ち(負け)だと判定したら、最大の幅で探索を試してみる。
          alpha = -ScoreInfinite;
          beta = ScoreInfinite;
        }
        else if (beta <= bestScore) {
          beta = std::min(bestScore + delta, ScoreInfinite);
          delta += delta / 2;
        }
        else {
          signals.failedLowAtRoot = true;
          signals.stopOnPonderHit = false;

          alpha = std::max(bestScore - delta, -ScoreInfinite);
          delta += delta / 2;
        }

        assert(-ScoreInfinite <= alpha && beta <= ScoreInfinite);
      }

      insertionSort(rootMoves.begin(), rootMoves.begin() + pvIdx + 1);
    }

    //if (skill.enabled() && skill.timeToPick(depth)) {
    //	skill.pickMove();
    //}

    // 以下の条件下で反復深化を打ち切る
    // - 持ち時間が残っている
    // - 最善手がしばらく変わっていない
    if (timeManager->isTimeLeft() && !signals.stopOnPonderHit) {
      bool stop = false;

      if (4 < depth && depth < 50 && pvSize == 1) {
        timeManager->setSearchStatus(bestMoveChanges, prevBestMoveChanges, bestScore);
      }

      // 次のイテレーションを回す時間が無いなら、ストップ
      if ((timeManager->getSoftTimeLimitMs() * 62) / 100 < searchTimer.elapsed()) {
        stop = true;
      }

      if (2 < depth && bestMoveChanges)
        bestMoveNeverChanged = false;

      // 最善手が、ある程度の深さまで同じであれば、
      // その手が突出して良い手なのだろう。
      if (12 <= depth
        && !stop
        && bestMoveNeverChanged
        && pvSize == 1
        // ここは確実にバグらせないようにする。
        && -ScoreInfinite + 2 * CapturePawnScore <= bestScore
        && (rootMoves.size() == 1
          || timeManager->getSoftTimeLimitMs() * 40 / 100 < searchTimer.elapsed()))
      {
        const Score rBeta = bestScore - 2 * CapturePawnScore;
        (ss + 1)->staticEvalRaw.p[0][0] = ScoreNotEvaluated;
        (ss + 1)->excludedMove = rootMoves[0].pv_[0];
        (ss + 1)->skipNullMove = true;
        const Score s = search<NonPV>(pos, ss + 1, rBeta - 1, rBeta, (depth - 3) * OnePly, true);
        (ss + 1)->skipNullMove = false;
        (ss + 1)->excludedMove = Move::moveNone();

        if (s < rBeta) {
          stop = true;
        }
      }

      if (stop) {
        if (limits.ponder) {
          signals.stopOnPonderHit = true;
        }
        else {
          signals.stop = true;
        }
      }
    }
  }
  skill.swapIfEnabled(thisptr);

  if (outputInfo) {
    SYNCCOUT << pvInfoToUSI(pos, depth, alpha, beta) << SYNCENDL;
  }

}

#if defined INANIWA_SHIFT
// 稲庭判定
void Searcher::detectInaniwa(const Position& pos) {
  if (inaniwaFlag == NotInaniwa && 20 <= pos.gamePly()) {
    const Rank TRank7 = (pos.turn() == Black ? Rank7 : Rank3); // not constant
    const Bitboard mask = rankMask(TRank7) & ~fileMask<FileA>() & ~fileMask<FileI>();
    if ((pos.bbOf(Pawn, oppositeColor(pos.turn())) & mask) == mask) {
      inaniwaFlag = (pos.turn() == Black ? InaniwaIsWhite : InaniwaIsBlack);
      tt.clear();
    }
  }
}
#endif
#if defined BISHOP_IN_DANGER
void Searcher::detectBishopInDanger(const Position& pos) {
  if (bishopInDangerFlag == NotBishopInDanger && pos.gamePly() <= 50) {
    const Color them = oppositeColor(pos.turn());
    if (pos.hand(pos.turn()).exists<HBishop>()
      && pos.bbOf(Silver, them).isSet(inverseIfWhite(them, H3))
      && (pos.bbOf(King, them).isSet(inverseIfWhite(them, F2))
        || pos.bbOf(King, them).isSet(inverseIfWhite(them, F3))
        || pos.bbOf(King, them).isSet(inverseIfWhite(them, E1)))
      && pos.bbOf(Pawn, them).isSet(inverseIfWhite(them, G3))
      && pos.piece(inverseIfWhite(them, H2)) == Empty
      && pos.piece(inverseIfWhite(them, G2)) == Empty
      && pos.piece(inverseIfWhite(them, G1)) == Empty)
    {
      bishopInDangerFlag = (pos.turn() == Black ? BlackBishopInDangerIn28 : WhiteBishopInDangerIn28);
      //tt.clear();
    }
    else if (pos.hand(pos.turn()).exists<HBishop>()
      && pos.hand(them).exists<HBishop>()
      && pos.piece(inverseIfWhite(them, C2)) == Empty
      && pos.piece(inverseIfWhite(them, C1)) == Empty
      && pos.piece(inverseIfWhite(them, D2)) == Empty
      && pos.piece(inverseIfWhite(them, D1)) == Empty
      && pos.piece(inverseIfWhite(them, A2)) == Empty
      && (pieceToPieceType(pos.piece(inverseIfWhite(them, C3))) == Silver
        || pieceToPieceType(pos.piece(inverseIfWhite(them, B2))) == Silver)
      && (pieceToPieceType(pos.piece(inverseIfWhite(them, C3))) == Knight
        || pieceToPieceType(pos.piece(inverseIfWhite(them, B1))) == Knight)
      && ((pieceToPieceType(pos.piece(inverseIfWhite(them, E2))) == Gold
        && pieceToPieceType(pos.piece(inverseIfWhite(them, E1))) == King)
        || pieceToPieceType(pos.piece(inverseIfWhite(them, E1))) == Gold))
    {
      bishopInDangerFlag = (pos.turn() == Black ? BlackBishopInDangerIn78 : WhiteBishopInDangerIn78);
      //tt.clear();
    }
  }
}
#endif
template <bool DO> void Position::doNullMove(StateInfo& backUpSt) {
  assert(!inCheck());

  StateInfo* src = (DO ? st_ : &backUpSt);
  StateInfo* dst = (DO ? &backUpSt : st_);

  dst->boardKey = src->boardKey;
  dst->handKey = src->handKey;
  dst->pliesFromNull = src->pliesFromNull;
  dst->hand = hand(turn());
  turn_ = oppositeColor(turn());

  if (DO) {
    st_->boardKey ^= zobTurn();
    prefetch(csearcher()->tt.first_entry(st_->key()));
    st_->pliesFromNull = 0;
    st_->continuousCheck[turn()] = 0;
  }
  st_->hand = hand(turn());

  assert(isOK());
}

template <NodeType NT>
Score Searcher::search(Position& pos, SearchStack* ss, Score alpha, Score beta, const Depth depth, const bool cutNode) {
  constexpr bool PVNode = (NT == PV || NT == Root || NT == SplitPointPV || NT == SplitPointRoot);
  constexpr bool SPNode = (NT == SplitPointPV || NT == SplitPointNonPV || NT == SplitPointRoot);
  constexpr bool RootNode = (NT == Root || NT == SplitPointRoot);

  assert(-ScoreInfinite <= alpha && alpha < beta && beta <= ScoreInfinite);
  assert(PVNode || (alpha == beta - 1));
  assert(Depth0 < depth);

  // 途中で goto を使用している為、先に全部の変数を定義しておいた方が安全。
  Move movesSearched[64];
  StateInfo st;
  TTEntry* tte;
  SplitPoint* splitPoint;
  Key posKey;
  Move ttMove;
  Move move;
  Move excludedMove;
  Move bestMove;
  Move threatMove;
  Depth newDepth;
  Depth extension;
  Score bestScore;
  Score score;
  Score ttScore;
  Score eval;
  bool inCheck;
  bool givesCheck;
  bool isPVMove;
  bool singularExtensionNode;
  bool captureOrPawnPromotion;
  bool dangerous;
  bool doFullDepthSearch;
  int moveCount;
  int playedMoveCount;
  bool ttHit;

  // step1
  // initialize node
  Thread* thisThread = pos.thisThread();
  moveCount = playedMoveCount = 0;
  inCheck = pos.inCheck();

  if (SPNode) {
    splitPoint = ss->splitPoint;
    bestMove = splitPoint->bestMove;
    threatMove = splitPoint->threatMove;
    bestScore = splitPoint->bestScore;
    tte = nullptr;
    ttMove = excludedMove = Move::moveNone();
    ttScore = ScoreNone;

    evaluate(pos, ss);

    assert(-ScoreInfinite < splitPoint->bestScore && 0 < splitPoint->moveCount);

    goto split_point_start;
  }

  bestScore = -ScoreInfinite;
  ss->currentMove = threatMove = bestMove = (ss + 1)->excludedMove = Move::moveNone();
  ss->ply = (ss - 1)->ply + 1;
  (ss + 1)->skipNullMove = false;
  (ss + 1)->reduction = Depth0;
  (ss + 2)->killers[0] = (ss + 2)->killers[1] = Move::moveNone();

  if (PVNode && thisThread->maxPly < ss->ply) {
    thisThread->maxPly = ss->ply;
  }

  if (!RootNode) {
    // step2
    // stop と最大探索深さのチェック
    switch (pos.isDraw(16)) {
    case NotRepetition: if (!signals.stop && ss->ply <= MaxPly) { break; }
    case RepetitionDraw: return ScoreDraw;
    case RepetitionWin: return mateIn(ss->ply);
    case RepetitionLose: return matedIn(ss->ply);
    case RepetitionSuperior: if (ss->ply != 2) { return ScoreMateInMaxPly; } break;
    case RepetitionInferior: if (ss->ply != 2) { return ScoreMatedInMaxPly; } break;
    default: UNREACHABLE;
    }

    // step3
    // mate distance pruning
    if (!RootNode) {
      alpha = std::max(matedIn(ss->ply), alpha);
      beta = std::min(mateIn(ss->ply + 1), beta);
      if (beta <= alpha) {
        return alpha;
      }
    }
  }

  pos.setNodesSearched(pos.nodesSearched() + 1);

  // step4
  // trans position table lookup
  excludedMove = ss->excludedMove;
  posKey = (excludedMove.isNone() ? pos.getKey() : pos.getExclusionKey());
  tte = tt.probe(posKey, ttHit);
  ttMove =
    RootNode ? rootMoves[pvIdx].pv_[0] :
    ttHit ?
    move16toMove(tte->move(), pos) :
    Move::moveNone();
  ttScore = (ttHit ? scoreFromTT(tte->score(), ss->ply) : ScoreNone);

  if (!RootNode
    && ttHit
    && depth <= tte->depth()
    && ttScore != ScoreNone // アクセス競合が起きたときのみ、ここに引っかかる。
    && (PVNode ? tte->bound() == BoundExact
      : (beta <= ttScore ? (tte->bound() & BoundLower)
        : (tte->bound() & BoundUpper))))
  {
    //tt.refresh(tte);
    ss->currentMove = ttMove; // Move::moveNone() もありえる。

    if (beta <= ttScore
      && !ttMove.isNone()
      && !ttMove.isCaptureOrPawnPromotion()
      && ttMove != ss->killers[0])
    {
      ss->killers[1] = ss->killers[0];
      ss->killers[0] = ttMove;
    }
    return ttScore;
  }

#if 1
  if (!RootNode
    && !inCheck)
  {
    if (!(move = pos.mateMoveIn1Ply()).isNone()) {
      ss->staticEval = bestScore = mateIn(ss->ply);
      tte->save(posKey, scoreToTT(bestScore, ss->ply), BoundExact, depth,
        move, ss->staticEval, tt.generation());
      bestMove = move;
      return bestScore;
    }
  }
#endif

  // step5
  // evaluate the position statically
  eval = ss->staticEval = evaluate(pos, ss); // Bonanza の差分評価の為、evaluate() を常に呼ぶ。
  if (inCheck) {
    eval = ss->staticEval = ScoreNone;
    goto iid_start;
  }
  else if (ttHit) {
    if (ttScore != ScoreNone
      && (tte->bound() & (eval < ttScore ? BoundLower : BoundUpper)))
    {
      eval = ttScore;
    }
  }
  else {
    tte->save(posKey, ScoreNone, BoundNone, DepthNone,
      Move::moveNone(), ss->staticEval, tt.generation());
  }

  // 一手前の指し手について、history を更新する。
  // todo: ここの条件はもう少し考えた方が良い。
  if ((move = (ss - 1)->currentMove) != Move::moveNull()
    && (ss - 1)->staticEval != ScoreNone
    && ss->staticEval != ScoreNone
    && !move.isCaptureOrPawnPromotion() // 前回(一手前)の指し手が駒取りでなかった。
    )
  {
    const Square to = move.to();
    gains.update(move.isDrop(), pos.piece(to), to, -(ss - 1)->staticEval - ss->staticEval);
  }

  // step6
  // razoring
  if (!PVNode
    && depth < 4 * OnePly
    && eval + razorMargin(depth) < beta
    && ttMove.isNone()
    && abs(beta) < ScoreMateInMaxPly)
  {
    const Score rbeta = beta - razorMargin(depth);
    const Score s = qsearch<NonPV, false>(pos, ss, rbeta - 1, rbeta, Depth0);
    if (s < rbeta) {
      return s;
    }
  }

  // step7
  // static null move pruning
  if (!PVNode
    && !ss->skipNullMove
    && depth < 4 * OnePly
    && beta <= eval - FutilityMargins[depth][0]
    && abs(beta) < ScoreMateInMaxPly)
  {
    return eval - FutilityMargins[depth][0];
  }

  // step8
  // null move
  if (!PVNode
    && !ss->skipNullMove
    && 2 * OnePly <= depth
    && beta <= eval
    && abs(beta) < ScoreMateInMaxPly)
  {
    ss->currentMove = Move::moveNull();
    Depth reduction = static_cast<Depth>(3) * OnePly + depth / 4;

    if (beta < eval - PawnScore) {
      reduction += OnePly;
    }

    pos.doNullMove<true>(st);
    (ss + 1)->staticEvalRaw = (ss)->staticEvalRaw; // 評価値の差分評価の為。
    (ss + 1)->skipNullMove = true;
    Score nullScore = (depth - reduction < OnePly ?
      -qsearch<NonPV, false>(pos, ss + 1, -beta, -alpha, Depth0)
      : -search<NonPV>(pos, ss + 1, -beta, -alpha, depth - reduction, !cutNode));
    (ss + 1)->skipNullMove = false;
    pos.doNullMove<false>(st);

    if (beta <= nullScore) {
      if (ScoreMateInMaxPly <= nullScore) {
        nullScore = beta;
      }

      if (depth < 6 * OnePly) {
        return nullScore;
      }

      ss->skipNullMove = true;
      assert(Depth0 < depth - reduction);
      const Score s = search<NonPV>(pos, ss, alpha, beta, depth - reduction, false);
      ss->skipNullMove = false;

      if (beta <= s) {
        return nullScore;
      }
    }
    else {
      // fail low
      threatMove = (ss + 1)->currentMove;
      if (depth < 5 * OnePly
        && (ss - 1)->reduction != Depth0
        && !threatMove.isNone()
        && allows(pos, (ss - 1)->currentMove, threatMove))
      {
        return beta - 1;
      }
    }
  }

  // step9
  // probcut
  if (!PVNode
    && 5 * OnePly <= depth
    && !ss->skipNullMove
    // 確実にバグらせないようにする。
    && abs(beta) < ScoreInfinite - 200)
  {
    const Score rbeta = beta + 200;
    const Depth rdepth = depth - OnePly - 3 * OnePly;

    assert(OnePly <= rdepth);
    assert(!(ss - 1)->currentMove.isNone());
    assert((ss - 1)->currentMove != Move::moveNull());

    assert(move == (ss - 1)->currentMove);
    // move.cap() は前回(一手前)の指し手で取った駒の種類
    MovePicker mp(pos, ttMove, history, move.cap());
    const CheckInfo ci(pos);
    while (!(move = mp.nextMove<false>()).isNone()) {
      if (pos.pseudoLegalMoveIsLegal<false, false>(move, ci.pinned)) {
        ss->currentMove = move;
        pos.doMove(move, st, ci, pos.moveGivesCheck(move, ci));
        (ss + 1)->staticEvalRaw.p[0][0] = ScoreNotEvaluated;
        score = -search<NonPV>(pos, ss + 1, -rbeta, -rbeta + 1, rdepth, !cutNode);
        pos.undoMove(move);
        if (rbeta <= score) {
          return score;
        }
      }
    }
  }

iid_start:
  // step10
  // internal iterative deepening
  if ((PVNode ? 5 * OnePly : 8 * OnePly) <= depth
    && ttMove.isNone()
    && (PVNode || (!inCheck && beta <= ss->staticEval + static_cast<Score>(256))))
  {
    //const Depth d = depth - 2 * OnePly - (PVNode ? Depth0 : depth / 4);
    const Depth d = (PVNode ? depth - 2 * OnePly : depth / 2);

    ss->skipNullMove = true;
    search<PVNode ? PV : NonPV>(pos, ss, alpha, beta, d, true);
    ss->skipNullMove = false;

    tte = tt.probe(posKey, ttHit);
    ttMove = (ttHit ?
      move16toMove(tte->move(), pos) :
      Move::moveNone());
  }

split_point_start:
  MovePicker mp(pos, ttMove, depth, history, ss, PVNode ? -ScoreInfinite : beta);
  const CheckInfo ci(pos);
  score = bestScore;
  singularExtensionNode =
    !RootNode
    && !SPNode
    && 8 * OnePly <= depth
    && !ttMove.isNone()
    && excludedMove.isNone()
    && (tte->bound() & BoundLower)
    && depth - 3 * OnePly <= tte->depth();

  // step11
  // Loop through moves
  while (!(move = mp.nextMove<SPNode>()).isNone()) {
    if (move == excludedMove) {
      continue;
    }

    if (RootNode
      && std::find(rootMoves.begin() + pvIdx,
        rootMoves.end(),
        move) == rootMoves.end())
    {
      continue;
    }

    if (SPNode) {
      if (!pos.pseudoLegalMoveIsLegal<false, false>(move, ci.pinned)) {
        continue;
      }
      moveCount = ++splitPoint->moveCount;
      splitPoint->mutex.unlock();
    }
    else {
      ++moveCount;
    }


    if (RootNode) {
      signals.firstRootMove = (moveCount == 1);
#if 0
      if (thisThread == threads.mainThread() && 3000 < searchTimer.elapsed()) {
        SYNCCOUT << "info depth " << depth / OnePly
          << " currmove " << move.toUSI()
          << " currmovenumber " << moveCount + pvIdx << SYNCENDL;
      }
#endif
    }

    extension = Depth0;
    captureOrPawnPromotion = move.isCaptureOrPawnPromotion();
    givesCheck = pos.moveGivesCheck(move, ci);
    dangerous = givesCheck; // todo: not implement

    // step12
    if (givesCheck && ScoreZero <= pos.seeSign(move))
    {
      extension = OnePly;
    }

    // singuler extension
    if (singularExtensionNode
      && extension == Depth0
      && move == ttMove
      && pos.pseudoLegalMoveIsLegal<false, false>(move, ci.pinned)
      && abs(ttScore) < ScoreKnownWin)
    {
      assert(ttScore != ScoreNone);

      const Score rBeta = ttScore - static_cast<Score>(depth);
      ss->excludedMove = move;
      ss->skipNullMove = true;
      score = search<NonPV>(pos, ss, rBeta - 1, rBeta, depth / 2, cutNode);
      ss->skipNullMove = false;
      ss->excludedMove = Move::moveNone();

      if (score < rBeta) {
        //extension = OnePly;
        extension = (beta <= rBeta ? OnePly + OnePly / 2 : OnePly);
      }
    }

    newDepth = depth - OnePly + extension;

    // step13
    // futility pruning
    if (!PVNode
      && !captureOrPawnPromotion
      && !inCheck
      && !dangerous
      //&& move != ttMove // 次の行がtrueならこれもtrueなので条件から省く。
      && ScoreMatedInMaxPly < bestScore)
    {
      assert(move != ttMove);
      // move count based pruning
      if (depth < 16 * OnePly
        && FutilityMoveCounts[depth] <= moveCount
        && (threatMove.isNone() || !refutes(pos, move, threatMove)))
      {
        if (SPNode) {
          splitPoint->mutex.lock();
        }
        continue;
      }

      // score based pruning
      const Depth predictedDepth = newDepth - reduction<PVNode>(depth, moveCount);
      // gain を 2倍にする。
      const Score futilityScore = ss->staticEval + futilityMargin(predictedDepth, moveCount)
        + 2 * gains.value(move.isDrop(), colorAndPieceTypeToPiece(pos.turn(), move.pieceTypeFromOrDropped()), move.to());

      if (futilityScore < beta) {
        bestScore = std::max(bestScore, futilityScore);
        if (SPNode) {
          splitPoint->mutex.lock();
          if (splitPoint->bestScore < bestScore) {
            splitPoint->bestScore = bestScore;
          }
        }
        continue;
      }

      if (predictedDepth < 4 * OnePly
        && pos.seeSign(move) < ScoreZero)
      {
        if (SPNode) {
          splitPoint->mutex.lock();
        }
        continue;
      }
    }

    // RootNode, SPNode はすでに合法手であることを確認済み。
    if (!RootNode && !SPNode && !pos.pseudoLegalMoveIsLegal<false, false>(move, ci.pinned)) {
      --moveCount;
      continue;
    }

    isPVMove = (PVNode && moveCount == 1);
    ss->currentMove = move;
    if (!SPNode && !captureOrPawnPromotion && playedMoveCount < 64) {
      movesSearched[playedMoveCount++] = move;
    }

    // step14
    pos.doMove(move, st, ci, givesCheck);
    (ss + 1)->staticEvalRaw.p[0][0] = ScoreNotEvaluated;

    // step15
    // LMR
    if (3 * OnePly <= depth
      && !isPVMove
      && !captureOrPawnPromotion
      && move != ttMove
      && ss->killers[0] != move
      && ss->killers[1] != move)
    {
      ss->reduction = reduction<PVNode>(depth, moveCount);
      if (!PVNode && cutNode) {
        ss->reduction += OnePly;
      }
      const Depth d = std::max(newDepth - ss->reduction, OnePly);
      if (SPNode) {
        alpha = splitPoint->alpha;
      }
      // PVS
      score = -search<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, d, true);

      doFullDepthSearch = (alpha < score && ss->reduction != Depth0);
      ss->reduction = Depth0;
    }
    else {
      doFullDepthSearch = !isPVMove;
    }

    // step16
    // full depth search
    // PVS
    if (doFullDepthSearch) {
      if (SPNode) {
        alpha = splitPoint->alpha;
      }
      score = (newDepth < OnePly ?
        (givesCheck ? -qsearch<NonPV, true>(pos, ss + 1, -(alpha + 1), -alpha, Depth0)
          : -qsearch<NonPV, false>(pos, ss + 1, -(alpha + 1), -alpha, Depth0))
        : -search<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, newDepth, !cutNode));
    }

    // 通常の探索
    if (PVNode && (isPVMove || (alpha < score && (RootNode || score < beta)))) {
      score = (newDepth < OnePly ?
        (givesCheck ? -qsearch<PV, true>(pos, ss + 1, -beta, -alpha, Depth0)
          : -qsearch<PV, false>(pos, ss + 1, -beta, -alpha, Depth0))
        : -search<PV>(pos, ss + 1, -beta, -alpha, newDepth, false));
    }

    // step17
    pos.undoMove(move);

    assert(-ScoreInfinite < score && score < ScoreInfinite);

    // step18
    if (SPNode) {
      splitPoint->mutex.lock();
      bestScore = splitPoint->bestScore;
      alpha = splitPoint->alpha;
    }

    if (signals.stop || thisThread->cutoffOccurred()) {
      return score;
    }

    if (RootNode) {
      RootMove& rm = *std::find(rootMoves.begin(), rootMoves.end(), move);
      if (isPVMove || alpha < score) {
        // PV move or new best move
        rm.score_ = score;
#if defined BISHOP_IN_DANGER
        if ((bishopInDangerFlag == BlackBishopInDangerIn28 && move.toCSA() == "0082KA")
          || (bishopInDangerFlag == WhiteBishopInDangerIn28 && move.toCSA() == "0028KA")
          || (bishopInDangerFlag == BlackBishopInDangerIn78 && move.toCSA() == "0032KA")
          || (bishopInDangerFlag == WhiteBishopInDangerIn78 && move.toCSA() == "0078KA"))
        {
          rm.score_ -= options[OptionNames::DANGER_DEMERIT_SCORE];
        }
#endif
        rm.extractPvFromTT(pos);

        if (!isPVMove) {
          ++bestMoveChanges;
        }
      }
      else {
        rm.score_ = -ScoreInfinite;
      }
    }

    if (bestScore < score) {
      bestScore = (SPNode ? splitPoint->bestScore = score : score);

      if (alpha < score) {
        bestMove = (SPNode ? splitPoint->bestMove = move : move);

        if (PVNode && score < beta) {
          alpha = (SPNode ? splitPoint->alpha = score : score);
        }
        else {
          // fail high
          if (SPNode) {
            splitPoint->cutoff = true;
          }
          break;
        }
      }
    }

    // step19
    if (!SPNode
      && threads.minSplitDepth() <= depth
      && threads.availableSlave(thisThread)
      && thisThread->splitPointsSize < MaxSplitPointsPerThread)
    {
      assert(bestScore < beta);
      thisThread->split<FakeSplit>(pos, ss, alpha, beta, bestScore, bestMove,
        depth, threatMove, moveCount, mp, NT, cutNode);
      if (beta <= bestScore) {
        break;
      }
    }
  }

  if (SPNode) {
    return bestScore;
  }

  // step20
  if (moveCount == 0) {
    return !excludedMove.isNone() ? alpha : matedIn(ss->ply);
  }

  if (bestScore == -ScoreInfinite) {
    assert(playedMoveCount == 0);
    bestScore = alpha;
  }

  if (beta <= bestScore) {
    // failed high
    tte->save(posKey, scoreToTT(bestScore, ss->ply), BoundLower, depth,
      bestMove, ss->staticEval, tt.generation());

    if (!bestMove.isCaptureOrPawnPromotion() && !inCheck) {
      if (bestMove != ss->killers[0]) {
        ss->killers[1] = ss->killers[0];
        ss->killers[0] = bestMove;
      }

      const Score bonus = static_cast<Score>(depth * depth);
      const Piece pc1 = colorAndPieceTypeToPiece(pos.turn(), bestMove.pieceTypeFromOrDropped());
      history.update(bestMove.isDrop(), pc1, bestMove.to(), bonus);

      for (int i = 0; i < playedMoveCount - 1; ++i) {
        const Move m = movesSearched[i];
        const Piece pc2 = colorAndPieceTypeToPiece(pos.turn(), m.pieceTypeFromOrDropped());
        history.update(m.isDrop(), pc2, m.to(), -bonus);
      }
    }
  }
  else {
    // failed low or PV search
    tte->save(posKey, scoreToTT(bestScore, ss->ply),
      ((PVNode && !bestMove.isNone()) ? BoundExact : BoundUpper),
      depth, bestMove, ss->staticEval, tt.generation());
  }

  assert(-ScoreInfinite < bestScore && bestScore < ScoreInfinite);

  return bestScore;
}

void RootMove::extractPvFromTT(Position& pos) {
  StateInfo state[MaxPlyPlus2];
  StateInfo* st = state;
  TTEntry* tte;
  Ply ply = 0;
  Move m = pv_[0];
  bool ttHit;

  assert(!m.isNone() && pos.moveIsPseudoLegal(m));

  pv_.clear();

  do {
    pv_.push_back(m);

    assert(pos.moveIsLegal(pv_[ply]));
    pos.doMove(pv_[ply++], *st++);
    tte = pos.searcher()->tt.probe(pos.getKey(), ttHit);
  } while (ttHit
    // このチェックは少し無駄。駒打ちのときはmove16toMove() 呼ばなくて良い。
    && pos.moveIsPseudoLegal(m = move16toMove(tte->move(), pos))
    && pos.pseudoLegalMoveIsLegal<false, false>(m, pos.pinnedBB())
    && ply < MaxPly
    && (!pos.isDraw(20) || ply < 6));

  pv_.push_back(Move::moveNone());
  while (ply) {
    pos.undoMove(pv_[--ply]);
  }
}

void RootMove::insertPvInTT(Position& pos) {
  StateInfo state[MaxPlyPlus2];
  StateInfo* st = state;
  TTEntry* tte;
  Ply ply = 0;

  do {
    bool ttHit;
    tte = pos.searcher()->tt.probe(pos.getKey(), ttHit);

    if (!ttHit
      || move16toMove(tte->move(), pos) != pv_[ply])
    {
      tte->save(pos.getKey(), ScoreNone, BoundNone, DepthNone, pv_[ply], ScoreNone, pos.searcher()->tt.generation());
    }

    assert(pos.moveIsLegal(pv_[ply]));
    pos.doMove(pv_[ply++], *st++);
  } while (!pv_[ply].isNone());

  while (ply) {
    pos.undoMove(pv_[--ply]);
  }
}

void initSearchTable() {
  // todo: パラメータは改善の余地あり。
  int d;  // depth (ONE_PLY == 2)
  int hd; // half depth (ONE_PLY == 1)
  int mc; // moveCount

          // Init reductions array
  for (hd = 1; hd < 64; hd++) {
    for (mc = 1; mc < 64; mc++) {
      double    pvRed = log(double(hd)) * log(double(mc)) / 3.0;
      double nonPVRed = 0.33 + log(double(hd)) * log(double(mc)) / 2.25;
      Reductions[1][hd][mc] = (int8_t)(pvRed >= 1.0 ? floor(pvRed * int(OnePly)) : 0);
      Reductions[0][hd][mc] = (int8_t)(nonPVRed >= 1.0 ? floor(nonPVRed * int(OnePly)) : 0);
    }
  }

  for (d = 1; d < 16; ++d) {
    for (mc = 0; mc < 64; ++mc) {
      FutilityMargins[d][mc] = static_cast<Score>(112 * static_cast<int>(log(static_cast<double>(d*d) / 2) / log(2.0) + 1.001)
        - 8 * mc + 45);
    }
  }

  // init futility move counts
  for (d = 0; d < 32; ++d) {
    FutilityMoveCounts[d] = static_cast<int>(3.001 + 0.3 * pow(static_cast<double>(d), 1.8));
  }
}

// 入玉勝ちかどうかを判定
bool nyugyoku(const Position& pos) {
  // CSA ルールでは、一 から 六 の条件を全て満たすとき、入玉勝ち宣言が出来る。

  // 一 宣言側の手番である。

  // この関数を呼び出すのは自分の手番のみとする。ponder では呼び出さない。

  const Color us = pos.turn();
  // 敵陣のマスク
  const Bitboard opponentsField = (us == Black ? inFrontMask<Black, Rank6>() : inFrontMask<White, Rank4>());

  // 二 宣言側の玉が敵陣三段目以内に入っている。
  if (!pos.bbOf(King, us).andIsNot0(opponentsField))
    return false;

  // 三 宣言側が、大駒5点小駒1点で計算して
  //     先手の場合28点以上の持点がある。
  //     後手の場合27点以上の持点がある。
  //     点数の対象となるのは、宣言側の持駒と敵陣三段目以内に存在する玉を除く宣言側の駒のみである。
  const Bitboard bigBB = pos.bbOf(Rook, Dragon, Bishop, Horse) & opponentsField & pos.bbOf(us);
  const Bitboard smallBB = (pos.bbOf(Pawn, Lance, Knight, Silver) | pos.goldsBB()) & opponentsField & pos.bbOf(us);
  const Hand hand = pos.hand(us);
  const int val = (bigBB.popCount() + hand.numOf<HRook>() + hand.numOf<HBishop>()) * 5
    + smallBB.popCount()
    + hand.numOf<HPawn>() + hand.numOf<HLance>() + hand.numOf<HKnight>()
    + hand.numOf<HSilver>() + hand.numOf<HGold>();
#if defined LAW_24
  if (val < 31)
    return false;
#else
  if (val < (us == Black ? 28 : 27))
    return false;
#endif

  // 四 宣言側の敵陣三段目以内の駒は、玉を除いて10枚以上存在する。

  // 玉は敵陣にいるので、自駒が敵陣に11枚以上あればよい。
  if ((pos.bbOf(us) & opponentsField).popCount() < 11)
    return false;

  // 五 宣言側の玉に王手がかかっていない。
  if (pos.inCheck())
    return false;

  // 六 宣言側の持ち時間が残っている。

  // 持ち時間が無ければ既に負けなので、何もチェックしない。

  return true;
}

void Searcher::think() {
  static Book book;
  Position& pos = rootPosition;
  timeManager.reset(new TimeManager(limits, pos.gamePly(), pos.turn(), thisptr));
  std::uniform_int_distribution<int> dist(options[OptionNames::MIN_BOOK_PLY], options[OptionNames::MAX_BOOK_PLY]);
  const Ply book_ply = dist(g_randomTimeSeed);

  bool nyugyokuWin = false;
#if defined LEARN
#else
  if (nyugyoku(pos)) {
    nyugyokuWin = true;
    goto finalize;
  }
#endif
  pos.setNodesSearched(0);

#if defined LEARN
  threads[0]->searching = true;
#else
  tt.resize(options[OptionNames::USI_HASH]); // operator int() 呼び出し。
  //if (outputInfo) {
  //  SYNCCOUT << "info string book_ply " << book_ply << SYNCENDL;
  //}
  // 定跡データベース
  if (options[OptionNames::OWNBOOK] && pos.gamePly() <= book_ply) {
    int numberOfRootMoves = rootMoves.size();
    std::vector<std::pair<Move, int> > movesInBook =
      book.enumerateMoves(pos, options[OptionNames::BOOK_FILE]);

    // 合法手以外を取り除く
    std::vector<RootMove> rootMovesInBook;
    for (const auto& move : movesInBook) {
      if (move.first.isNone()) {
        continue;
      }
      if (std::find(rootMoves.begin(), rootMoves.end(), move.first) == rootMoves.end()) {
        continue;
      }
      rootMovesInBook.push_back(RootMove(move.first));
    }

    // 定跡データベースにヒットした場合は、
    // それらの手の中かから次の一手を探索する
    if (!rootMovesInBook.empty()) {
      rootMoves = rootMovesInBook;

      // 持ち時間節約のため、持ち時間が残っている場合は思考時間を短くする
      if (limits.time[pos.turn()] != 0) {
        limits.time[Black] = 0;
        limits.time[White] = 0;
        limits.byoyomi = options[OptionNames::BOOK_THINKING_TIME];
        timeManager->update();
      }

      SYNCCOUT << "info string Reduced root moves " << numberOfRootMoves << " -> " << rootMovesInBook.size() << SYNCENDL;
    }
  }

  threads.wakeUp(thisptr);

  int timerPeriodFirstMs;
  int timerPeriodAfterMs;
  if (limits.nodes) {
    timerPeriodFirstMs = MIN_TIMER_PERIOD_MS;
    timerPeriodAfterMs = MIN_TIMER_PERIOD_MS;
  }
  else if (timeManager->isTimeLeft() || timeManager->isInByoyomi()) {
    // なるべく思考スレッドに処理時間を渡すため
    // 初回思考時間チェックは maximumTime の直前から行う
    timerPeriodFirstMs = timeManager->getHardTimeLimitMs() - MAX_TIMER_PERIOD_MS * 2;
    timerPeriodFirstMs = std::max(timerPeriodFirstMs, MIN_TIMER_PERIOD_MS);
    timerPeriodAfterMs = MAX_TIMER_PERIOD_MS;
    //SYNCCOUT << "info string *** think() : other" << SYNCENDL;
  }
  else {
    timerPeriodFirstMs = TimerThread::FOREVER;
    timerPeriodAfterMs = TimerThread::FOREVER;
  }

  threads.timerThread()->restartTimer(timerPeriodFirstMs, timerPeriodAfterMs);

#if defined INANIWA_SHIFT
  detectInaniwa(pos);
#endif
#if defined BISHOP_IN_DANGER
  detectBishopInDanger(pos);
#endif
#endif
  idLoop(pos);

#if defined LEARN
#else
  // timer を止める。
  threads.timerThread()->restartTimer(TimerThread::FOREVER, TimerThread::FOREVER);
  threads.sleep();

finalize:

  if (outputInfo) {
    SYNCCOUT << "info nodes " << pos.nodesSearched()
      << " time " << searchTimer.elapsed() << SYNCENDL;
  }
  lastSearchedNodes = pos.nodesSearched();

  if (!signals.stop && (limits.ponder || limits.infinite)) {
    signals.stopOnPonderHit = true;
    pos.thisThread()->waitFor(signals.stop);
  }

  if (outputInfo) {
    if (nyugyokuWin) {
      SYNCCOUT << "bestmove win" << SYNCENDL;
    }
    else if (rootMoves[0].pv_[0].isNone()) {
      SYNCCOUT << "bestmove resign" << SYNCENDL;
    }
    else {
      SYNCCOUT << "bestmove " << rootMoves[0].pv_[0].toUSI()
        << " ponder " << rootMoves[0].pv_[1].toUSI()
        << SYNCENDL;
    }
  }
#endif
}

void Searcher::checkTime() {
  if (limits.ponder)
    return;

  s64 nodes = 0;
  if (limits.nodes) {
    std::unique_lock<Mutex> lock(threads.mutex_);

    nodes = rootPosition.nodesSearched();
    for (size_t i = 0; i < threads.size(); ++i) {
      for (int j = 0; j < threads[i]->splitPointsSize; ++j) {
        SplitPoint& splitPoint = threads[i]->splitPoints[j];
        std::unique_lock<Mutex> spLock(splitPoint.mutex);
        nodes += splitPoint.nodes;
        u64 sm = splitPoint.slavesMask;
        while (sm) {
          const int index = firstOneFromLSB(sm);
          sm &= sm - 1;
          Position* pos = threads[index]->activePosition;
          if (pos != nullptr) {
            nodes += pos->nodesSearched();
          }
        }
      }
    }
  }

  const int elapsed = searchTimer.elapsed();
  const bool stillAtFirstMove =
    signals.firstRootMove
    && !signals.failedLowAtRoot
    && timeManager->getSoftTimeLimitMs() < elapsed;

  const bool noMoreTime =
    timeManager->getHardTimeLimitMs() - 2 * MIN_TIMER_PERIOD_MS < elapsed
    || stillAtFirstMove;

  if (noMoreTime || (limits.nodes != 0 && limits.nodes < nodes))
  {
    signals.stop = true;
  }
}

void Thread::idleLoop() {
  SplitPoint* thisSp = splitPointsSize ? activeSplitPoint.load() : nullptr;
  assert(!thisSp || (thisSp->masterThread == this && searching));

  while (true) {
    while ((!searching && searcher->threads.sleepWhileIdle_) || exit)
    {
      if (exit) {
        assert(thisSp == nullptr);
        return;
      }

      std::unique_lock<Mutex> lock(sleepLock);
      if (thisSp != nullptr && !thisSp->slavesMask) {
        break;
      }

      if (!searching && !exit) {
        sleepCond.wait(lock);
      }
    }

    if (searching) {
      assert(!exit);

      searcher->threads.mutex_.lock();
      assert(searching);
      SplitPoint* sp = activeSplitPoint;
      searcher->threads.mutex_.unlock();

      SearchStack ss[MaxPlyPlus2];
      Position pos(*sp->pos, this);

      memcpy(ss, sp->ss - 1, 4 * sizeof(SearchStack));
      (ss + 1)->splitPoint = sp;

      sp->mutex.lock();

      assert(activePosition == nullptr);

      activePosition = &pos;

      switch (sp->nodeType) {
      case Root: searcher->search<SplitPointRoot >(pos, ss + 1, sp->alpha, sp->beta, sp->depth, sp->cutNode); break;
      case PV: searcher->search<SplitPointPV   >(pos, ss + 1, sp->alpha, sp->beta, sp->depth, sp->cutNode); break;
      case NonPV: searcher->search<SplitPointNonPV>(pos, ss + 1, sp->alpha, sp->beta, sp->depth, sp->cutNode); break;
      default: UNREACHABLE;
      }

      assert(searching);
      searching = false;
      activePosition = nullptr;
      assert(sp->slavesMask & (UINT64_C(1) << idx));
      sp->slavesMask ^= (UINT64_C(1) << idx);
      sp->nodes += pos.nodesSearched();

      if (searcher->threads.sleepWhileIdle_
        && this != sp->masterThread
        && !sp->slavesMask)
      {
        assert(!sp->masterThread->searching);
        sp->masterThread->notifyOne();
      }
      sp->mutex.unlock();
    }

    if (thisSp != nullptr && !thisSp->slavesMask) {
      thisSp->mutex.lock();
      const bool finished = !thisSp->slavesMask;
      thisSp->mutex.unlock();
      if (finished) {
        return;
      }
    }
  }
}
