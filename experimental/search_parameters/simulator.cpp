#include <random>
#include <chrono>
#include <bitset>
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <vector>
#include <algorithm>
#include <functional>
#include <iterator>
#include <locale>
#include <memory>
#include <stdexcept>
#include <utility>
#include <string>
#include <fstream>
#include <ios>
#include <iostream>
#include <iosfwd>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <complex>
#include <numeric>
#include <valarray>
#include <exception>
#include <limits>
#include <new>
#include <typeinfo>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <climits>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdlib>
#include <cstddef>
#include <cstdarg>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cwctype>
using namespace std;
static const double EPS = 1e-8;
static const double PI = 4.0 * atan(1.0);
static const double PI2 = 8.0 * atan(1.0);
using ll = long long;
using ull = unsigned long long;

#define ALL(c) (c).begin(), (c).end()
#define CLEAR(v) memset(v,0,sizeof(v))
#define MP(a,b) make_pair((a),(b))
#define REP(i,n) for(int i=0, i##_len=(n); i<i##_len; ++i)
#define ABS(a) ((a)>0?(a):-(a))
template<class T> T MIN(const T& a, const T& b) { return a < b ? a : b; }
template<class T> T MAX(const T& a, const T& b) { return a > b ? a : b; }
template<class T> void MIN_UPDATE(T& a, const T& b) { if (a > b) a = b; }
template<class T> void MAX_UPDATE(T& a, const T& b) { if (a < b) a = b; }

constexpr int mo = 1000000007;
int qp(int a, ll b) { int ans = 1; do { if (b & 1)ans = 1ll * ans*a%mo; a = 1ll * a*a%mo; } while (b >>= 1); return ans; }
int qp(int a, ll b, int mo) { int ans = 1; do { if (b & 1)ans = 1ll * ans*a%mo; a = 1ll * a*a%mo; } while (b >>= 1); return ans; }
int gcd(int a, int b) { return b ? gcd(b, a%b) : a; }
int dx[4] = { 1,0,-1,0 };
int dy[4] = { 0,1,0,-1 };

namespace
{
  // 正解パラメーター時のレーティング
  static constexpr double RATE_BASELINE = 3000.0;
  // 正解パラメーター
  static std::vector<int> CORRECT_PARAMETERS;
  // パラメータ1.0に対する整数
  static constexpr int PARAMETER_SCALE = 1024;
  // パラメーターからレーティングへの変換係数
  static constexpr double PARAMETER_TO_RATE_SCALE = 0.005;
  // パラメーター数
  static constexpr int NUMBER_OF_PARAMETERS = 16;
  // 擬似試合数
  static constexpr int NUMBER_OF_GAMES = 1000;
  // パラメーターを一度にどのくらい変化させるか
  static constexpr double PARAMETER_CHANGE_RATIO = 0.1;
  // nPm
  static double P[1024][1024];
  // パラメーター変化を受理する有意差の閾値
  // 有意差がこれより高い場合に、変化後のパラメーターを受理する
  static double SIGNIFICANT_DIFFERENCE_THRESHOLD = 0.999;

  std::mt19937_64 MT;
  std::normal_distribution<> NORMAL_DISTRIBUTION;

  // レーティングから勝率を計算する
  // https://ja.wikipedia.org/wiki/%E3%82%A4%E3%83%AD%E3%83%AC%E3%83%BC%E3%83%86%E3%82%A3%E3%83%B3%E3%82%B0
  double calculateWinningRate(double Ra, double Rb) {
    return 1.0 / (1.00 + pow(10.0, (Rb - Ra) / 400.0));
  }

  // パラメーターから擬似レーティングを計算する
  double calculateRate(const vector<int>& parameters) {
    assert(CORRECT_PARAMETERS.size() == parameters.size());
    double rate = RATE_BASELINE;
    REP(i, parameters.size()) {
      double diff = parameters[i] - CORRECT_PARAMETERS[i];
      diff *= PARAMETER_TO_RATE_SCALE;
      rate -= diff * diff;
    }
    return rate;
  }
}

//TODO:状態を表す型/構造体を作成する
class STATE {
public:
  //TODO:コンストラクタに必要な引数を追加する
  explicit STATE();
  void next();
  void prev();
  double calculateTransitionProbability();

  std::vector<int> parameters;
  std::vector<int> prevParameters;
};

class SimulatedAnnealing {
public:
  //TODO:コンストラクタに必要な引数を追加する
  SimulatedAnnealing();
  STATE run();
private:
  double calculateProbability(double score, double scoreNeighbor, double temperature);
};

//TODO:コンストラクタに必要な引数を追加する
SimulatedAnnealing::SimulatedAnnealing() {
}

STATE SimulatedAnnealing::run() {
  const auto startTime = std::chrono::system_clock::now();
  STATE state;
  STATE result = state;
  int counter = 0;
  REP(loop, 100) {
    state.next();
    std::uniform_real_distribution<> dist;
    const double probability = state.calculateTransitionProbability();
    if (dist(MT) < probability) {
      //Accept
      result = state;
      fprintf(stderr, "受理: rating=%f\n", calculateRate(state.parameters));
    }
    else {
      //Decline
      state.prev();
      //fprintf(stderr, "棄却\n");
    }
    ++counter;
  }

  return result;
}

double SimulatedAnnealing::calculateProbability(double energy, double energyNeighbor, double temperature) {
  if (energyNeighbor < energy) {
    return 1;
  }
  else {
    const double result = exp((energy - energyNeighbor) / (temperature + 1e-9) * 1.0);
    //fprintf(stderr, "%lf -> %lf * %lf = %lf\n", energy, energyNeighbor, temperature, result);
    return result;
  }
}

// 初期状態を作る
STATE::STATE() {
  REP(i, NUMBER_OF_PARAMETERS) {
    parameters.push_back(int(NORMAL_DISTRIBUTION(MT) * PARAMETER_SCALE));
  }
  cout << "初期パラメーター:" << endl;
  copy(ALL(parameters), ostream_iterator<int>(cout, ", "));
  cout << endl;
  cout << "初期レーティング: " << calculateRate(parameters) << endl;
}

// 遷移後の状態を作る
void STATE::next() {
  prevParameters = parameters;
  std::uniform_int_distribution<> dist(0, NUMBER_OF_PARAMETERS - 1);
  int parameterIndex = dist(MT);
  int diff = int(NORMAL_DISTRIBUTION(MT) * PARAMETER_SCALE * PARAMETER_CHANGE_RATIO);
  parameters[parameterIndex] += diff;
}

// 遷移前の状態に戻す
void STATE::prev() {
  parameters = prevParameters;
}

// 遷移確率を計算する
double STATE::calculateTransitionProbability() {
  // 擬似試合を行い処理回数を計算する
  int numberOfWins = 0;
  // 勝率はパラメーター変化前と変化後のレーティングから計算する
  double winningRate = calculateWinningRate(
    calculateRate(parameters),
    calculateRate(prevParameters));
  std::uniform_real_distribution<> dist;
  REP(i, NUMBER_OF_GAMES) {
    if (dist(MT) < winningRate) {
      ++numberOfWins;
    }
  }

  // 有意差を計算する
  // 有意差は二項分布における0～numberOfWinsまでの値を足した数 / 2^(試合数)
  // …で合ってるのかなぁ。(要確認)
  double acc = 0.0;
  REP(i, numberOfWins + 1) {
    acc += P[NUMBER_OF_GAMES][i];
  }
  double significantDifference = acc / pow(2.0, NUMBER_OF_GAMES);

  // 有意差の閾値を超えていたら100%遷移する
  // そうでない場合は遷移しない
  if (significantDifference > SIGNIFICANT_DIFFERENCE_THRESHOLD) {
    return 1.0;
  }
  else {
    return 0.0;
  }
}

int main()
{
  P[0][0] = 1.0;
  for (int i = 1; i < 1024; ++i) {
    P[i][0] = 1.0;
    for (int j = 1; j < 1024; ++j) {
      P[i][j] = P[i - 1][j - 1] + P[i - 1][j];
    }
  }

  cout << "有意差の閾値: " << SIGNIFICANT_DIFFERENCE_THRESHOLD << endl;

  REP(i, NUMBER_OF_PARAMETERS) {
    CORRECT_PARAMETERS.push_back(int(NORMAL_DISTRIBUTION(MT) * PARAMETER_SCALE));
  }
  cout << "正解パラメーター:" << endl;
  copy(ALL(CORRECT_PARAMETERS), ostream_iterator<int>(cout, ", "));
  cout << endl;
  cout << "レーティング: " << calculateRate(CORRECT_PARAMETERS) << endl;

  //REP(loop, 10) {
  //  vector<int> parameters;
  //  REP(i, NUMBER_OF_PARAMETERS) {
  //    parameters.push_back(int(NORMAL_DISTRIBUTION(MT) * PARAMETER_SCALE));
  //  }
  //  cout << calculateRate(parameters) << endl;
  //}

  STATE result = SimulatedAnnealing().run();

  cout << "自動調整シミュレーション結果" << endl;
  copy(ALL(result.parameters), ostream_iterator<int>(cout, ", "));
  cout << endl;
  cout << "レーティング: " << calculateRate(result.parameters) << endl;
}
