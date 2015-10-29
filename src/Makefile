KPP_PADDING0 = 5
KPP_PADDING1 = 4
USI_HASH_FOR_BENCHMARK=\"8192\"
EVALUATE_TABLE_SIZE = 0x10000000LL
TARGET_PREFIX = tanuki-clang

COMPILER = clang-3.7
CFLAGS   = -flto -std=c++11 -fno-exceptions -fno-rtti -m64 -Wextra -Ofast -DKPP_PADDING0=$(KPP_PADDING0) -DKPP_PADDING1=$(KPP_PADDING1) -DUSI_HASH_FOR_BENCHMARK=$(USI_HASH_FOR_BENCHMARK) -DEVALUATE_TABLE_SIZE=$(EVALUATE_TABLE_SIZE) -DMY_NAME=\"$(TARGET_PREFIX)\" 
#CFLAGS   = -std=c++11 -fno-exceptions -fno-rtti -m64 -Wextra -O0 -g -DKPP_PADDING0=$(KPP_PADDING0) -DKPP_PADDING1=$(KPP_PADDING1) -DUSI_HASH_FOR_BENCHMARK=$(USI_HASH_FOR_BENCHMARK) -DEVALUATE_TABLE_SIZE=$(EVALUATE_TABLE_SIZE) -DMY_NAME=\"$(TARGET_PREFIX)\" 
LDFLAGS  = -flto -lpthread -lgcov
LIBS     =
INCLUDE  = #-I../include
ifeq ($(OS),Windows_NT)
  TARGET = $(TARGET_PREFIX).exe
  LDFLAGS += -static
else
  TARGET = $(TARGET_PREFIX)
endif
OBJDIR   = ../obj
ifeq "$(strip $(OBJDIR))" ""
  OBJDIR = ..
endif
#SOURCES  = $(wildcard *.cpp)
SOURCES  = main.cpp bitboard.cpp init.cpp position.cpp evalList.cpp \
           move.cpp movePicker.cpp square.cpp usi.cpp generateMoves.cpp evaluate.cpp \
           search.cpp hand.cpp tt.cpp timeManager.cpp book.cpp benchmark.cpp \
           thread.cpp common.cpp pieceScore.cpp scanner.cpp string_util.cpp limits_type.cpp
OBJECTS  = $(addprefix $(OBJDIR)/, $(SOURCES:.cpp=.o))
DEPENDS  = $(OBJECTS:.o=.d)
PGO_TARGET = native

$(TARGET): $(OBJECTS) $(LIBS)
	$(COMPILER) -o $@ $^ $(LDFLAGS) $(CFLAGS)

$(OBJDIR)/%.o: %.cpp
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(COMPILER) $(CFLAGS) $(INCLUDE) -o $@ -c $<

all: clean $(TARGET)

native:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -DHAVE_BMI2 -DHAVE_AVX2 -march=native' LDFLAGS='$(LDFLAGS)' $(TARGET)

broadwell:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -DHAVE_BMI2 -DHAVE_AVX2 -march=broadwell' LDFLAGS='$(LDFLAGS)' $(TARGET)

avx2:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -DHAVE_BMI2 -DHAVE_AVX2 -mavx2 -mbmi2' LDFLAGS='$(LDFLAGS)' $(TARGET)

bmi2:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -DHAVE_BMI2 -msse4.2 -mbmi2' LDFLAGS='$(LDFLAGS)' $(TARGET)

sse:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2' LDFLAGS='$(LDFLAGS)' $(TARGET)

sse41:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DHAVE_SSE4 -msse4.1' LDFLAGS='$(LDFLAGS)' $(TARGET)

nosse:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG ' LDFLAGS='$(LDFLAGS)' $(TARGET)

prof:
	$(MAKE) CFLAGS='$(CFLAGS) -pg' bmi2

profgen:
	$(MAKE) CFLAGS='$(CFLAGS) -fprofile-generate' LDFLAGS='$(LDFLAGS) -fprofile-generate' $(PGO_TARGET)

profuse:
	$(MAKE) CFLAGS='$(CFLAGS) -fprofile-use -fprofile-correction' LDFLAGS='$(LDFLAGS) -fprofile-use -fprofile-correction' $(PGO_TARGET)

pgo:
	$(MAKE) profgen
	@./$(TARGET) bench | grep "info nodes"
	# llvm-profdata-3.7 merge -output=default.profdata *.profraw
	@touch *.[ch]pp
	$(MAKE) profuse

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(TARGET) ${OBJECTS:.o=.gcda}

-include $(DEPENDS)
