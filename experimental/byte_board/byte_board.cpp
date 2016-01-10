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

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using ymm = __m256i;
using xmm = __m128i;

struct ByteBoard
{
  union {
    ymm y[3];
    alignas(32) u8 b[81];
  };
};

struct BitBoard
{
  union {
    xmm x;
    alignas(32) u8 b[16];
    alignas(32) u16 s[8];
    alignas(32) u32 i[4];
    alignas(32) u64 l[2];
  };
};

int main()
{
  ByteBoard b;
  for (int i = 0; i < 96; ++i) {
    b.b[i] = i % 3;
  }

  ymm zero = _mm256_setzero_si256();
  ymm y0 = _mm256_load_si256(&b.y[0]);
  ymm mask0 = _mm256_cmpgt_epi8(y0, zero);
  int m0 = _mm256_movemask_epi8(mask0);

  ymm y1 = _mm256_load_si256(&b.y[1]);
  ymm mask1 = _mm256_cmpgt_epi8(y1, zero);
  int m1 = _mm256_movemask_epi8(mask1);

  ymm y2 = _mm256_load_si256(&b.y[2]);
  ymm mask2 = _mm256_cmpgt_epi8(y2, zero);
  int m2 = _mm256_movemask_epi8(mask2);

  BitBoard bitBoard0;
  bitBoard0.x = _mm_set_epi32(0, m2, m1, m0);
  for (int i = 0; i < 16; ++i) {
    std::cout << (int)bitBoard0.b[i] << std::endl;
  }

  BitBoard bitBoard1;
  bitBoard1.x = _mm_set_epi32(0, (m2 << 1) | (u32(m1) >> 31), m1 & 0x7fffffff, m0);
  for (int i = 0; i < 16; ++i) {
    std::cout << (int)bitBoard1.b[i] << std::endl;
  }
}
