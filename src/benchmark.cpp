#include "benchmark.hpp"
#include "common.hpp"
#include "usi.hpp"
#include "position.hpp"
#include "search.hpp"
#include "thread.hpp"

extern bool showInfo;

void setPosition(Position& pos, std::istringstream& ssCmd);
void go(const Position& pos, std::istringstream& ssCmd);

// 今はベンチマークというより、PGO ビルドの自動化の為にある。
void benchmark(Position& pos) {
  showInfo = false;

	std::string token;
	LimitsType limits;

  g_options["Threads"] = std::string("8");
  g_options["USI_Hash"] = std::string("8192");

	std::ifstream ifs("../src/benchmark.sfen");
  assert(ifs.is_open());
	std::string sfen;
	while (std::getline(ifs, sfen)) {
		std::cout << sfen << std::endl;
		std::istringstream ss_sfen(sfen);
		setPosition(pos, ss_sfen);
		std::istringstream ss_go("byoyomi 10000");
		go(pos, ss_go);
    g_threads.waitForThinkFinished();
	}
}
