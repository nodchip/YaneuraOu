#include <gtest/gtest.h>
#include "init.hpp"
#include "thread.hpp"
#include "usi.hpp"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  std::cout << engine_name() << std::endl;

  initTable(true);
  Position::initZobrist();
  g_threads.init();
  Searcher::tt.setSize(g_options["USI_Hash"]);

  int statusCode = RUN_ALL_TESTS();

  g_threads.exit(); // main関数が終わるまでにスレッドは終了させる

  return statusCode;
}
