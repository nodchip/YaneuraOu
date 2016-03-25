#include "common.hpp"
#include "bitboard.hpp"
#include "init.hpp"
#include "position.hpp"
#include "usi.hpp"
#include "thread.hpp"
#include "tt.hpp"
#include "search.hpp"

#if defined(DUMP_MINIDUMP_ON_EXCEPTION)
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

#if defined FIND_MAGIC
// Magic Bitboard の Magic Number を求める為のソフト
int main_sub(int argc, char* argv[]) {
  u64 RookMagic[SquareNum];
  u64 BishopMagic[SquareNum];

  std::cout << "const u64 RookMagic[81] = {" << std::endl;
  for (Square sq = I9; sq < SquareNum; ++sq) {
    RookMagic[sq] = findMagic(sq, false);
    std::cout << "\tUINT64_C(0x" << std::hex << RookMagic[sq] << ")," << std::endl;
  }
  std::cout << "};\n" << std::endl;

  std::cout << "const u64 BishopMagic[81] = {" << std::endl;
  for (Square sq = I9; sq < SquareNum; ++sq) {
    BishopMagic[sq] = findMagic(sq, true);
    std::cout << "\tUINT64_C(0x" << std::hex << BishopMagic[sq] << ")," << std::endl;
  }
  std::cout << "};\n" << std::endl;

  return 0;
}

#elif 0
// InFrontOfRank9Black
int main_sub(int argc, char* argv[]) {
  for (Color color = Black; color <= White; ++color) {
    for (Rank rank = Rank9; rank <= Rank1; ++rank) {
      Bitboard bb = inFrontMask(color, rank);
      printf("const Bitboard InFrontOfRank%d%s(UINT64_C(0x%016I64x), UINT64_C(0x%016I64x));\n",
        9 - rank, color == Black ? "Black" : "White", bb.p(0), bb.p(1));
    }
  }
}

#else
// 将棋を指すソフト
int main_sub(int argc, char* argv[]) {
  initTable();
  Position::initZobrist();
  auto s = std::unique_ptr<Searcher>(new Searcher);
  s->init();
  // 一時オブジェクトの生成と破棄
  std::unique_ptr<Evaluater>(new Evaluater)->init(USI::Options[OptionNames::EVAL_DIR], true);
  s->doUSICommandLoop(argc, argv);
  s->threads.exit();
}

#endif

#ifdef DUMP_MINIDUMP_ON_EXCEPTION
void create_minidump(_EXCEPTION_POINTERS* pep) {
  std::wostringstream oss;
  oss << L"tanuki-." << GetCurrentProcessId() << L".dmp";
  std::wstring file_name = oss.str();

  HANDLE hFile = CreateFile(file_name.c_str(), GENERIC_READ | GENERIC_WRITE, 
    0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 

  if (hFile != NULL && hFile != INVALID_HANDLE_VALUE) {
    MINIDUMP_EXCEPTION_INFORMATION mdei; 

    mdei.ThreadId = GetCurrentThreadId(); 
    mdei.ExceptionPointers = pep; 
    mdei.ClientPointers = FALSE; 

    const MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpNormal
      | MiniDumpWithFullMemory
      | MiniDumpWithFullMemoryInfo
      | MiniDumpWithHandleData
      | MiniDumpWithProcessThreadData
      | MiniDumpWithThreadInfo
      | MiniDumpWithUnloadedModules
      );

    auto rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
      hFile, mdt, (pep != 0) ? &mdei : 0, 0, 0); 

    if (rv) {
      SYNCCOUT << "info string Created minidump to: " << file_name.c_str() << SYNCENDL;
    } else {
      SYNCCOUT << "info string Failed to create minidump" << SYNCENDL;
    }

    CloseHandle(hFile); 
  } else {
    SYNCCOUT << "info string Failed to open file for minidump" << SYNCENDL;
  }
}

int main(int argc, char* argv[]) {
  __try {
    return main_sub(argc, argv);
  } __except(
    create_minidump(GetExceptionInformation()),
    EXCEPTION_EXECUTE_HANDLER) {
  }
  return -1;
}
#else // DUMP_MINIDUMP_ON_EXCEPTION
int main(int argc, char* argv[]) {
  return main_sub(argc, argv);
}
#endif // DUMP_MINIDUMP_ON_EXCEPTION
