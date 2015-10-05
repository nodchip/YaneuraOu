KPP_PADDING0 = 5
KPP_PADDING1 = 4
EVALUATE_TABLE_SIZE = 0x8000000LL

COMPILER = g++
#COMPILER = mpicxx
CFLAGS   = -std=c++11 -fno-exceptions -fno-rtti -m64 -Wextra -Ofast -DKPP_PADDING0=$(KPP_PADDING0) -DKPP_PADDING1=$(KPP_PADDING1) -DEVALUATE_TABLE_SIZE=$(EVALUATE_TABLE_SIZE) #-MMD -MP
#CFLAGS   = -std=c++11 -fno-exceptions -fno-rtti -m64 -Wextra -O0 -g -DKPP_PADDING0=$(KPP_PADDING0) -DKPP_PADDING1=$(KPP_PADDING1) -DEVALUATE_TABLE_SIZE=$(EVALUATE_TABLE_SIZE) #-MMD -MP
LDFLAGS  = -lpthread #-lboost_mpi -lboost_serialization
LIBS     =
INCLUDE  = #-I../include
TARGET_PREFIX = tanuki-gcc
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
SOURCES  = main.cpp bitboard.cpp init.cpp mt64bit.cpp position.cpp evalList.cpp \
           move.cpp movePicker.cpp square.cpp usi.cpp generateMoves.cpp evaluate.cpp \
           search.cpp hand.cpp tt.cpp timeManager.cpp book.cpp benchmark.cpp \
           thread.cpp common.cpp pieceScore.cpp scanner.cpp string_util.cpp
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
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DMY_NAME=\"$(TARGET_PREFIX)\" -DHAVE_SSE4 -DHAVE_SSE42 -DHAVE_BMI2 -DHAVE_AVX2 -march=native' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)

broadwell:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DMY_NAME=\"$(TARGET_PREFIX)\" -DHAVE_SSE4 -DHAVE_SSE42 -DHAVE_BMI2 -DHAVE_AVX2 -march=broadwell' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)

avx2:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DMY_NAME=\"$(TARGET_PREFIX)\" -DHAVE_SSE4 -DHAVE_SSE42 -DHAVE_BMI2 -DHAVE_AVX2 -mavx2 -mbmi2' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)

bmi2:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DMY_NAME=\"$(TARGET_PREFIX)\" -DHAVE_SSE4 -DHAVE_SSE42 -DHAVE_BMI2 -msse4.2 -mbmi2' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)

sse:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DMY_NAME=\"$(TARGET_PREFIX)\" -DHAVE_SSE4 -DHAVE_SSE42 -msse4.2' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)

sse41:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DMY_NAME=\"$(TARGET_PREFIX)\" -DHAVE_SSE4 -msse4.1' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)

nosse:
	$(MAKE) CFLAGS='$(CFLAGS) -DNDEBUG -DMY_NAME=\"$(TARGET_PREFIX)\"' LDFLAGS='$(LDFLAGS) -flto' $(TARGET)

prof:
	$(MAKE) CFLAGS='$(CFLAGS) -pg' bmi2

profgen:
	$(MAKE) CFLAGS='$(CFLAGS) -fprofile-generate -lgcov' LDFLAGS='$(LDFLAGS) -fprofile-generate -lgcov' $(PGO_TARGET)

profuse:
	$(MAKE) CFLAGS='$(CFLAGS) -fprofile-use -lgcov' LDFLAGS='$(LDFLAGS) -fprofile-use -lgcov' $(PGO_TARGET)

pgo:
	$(MAKE) profgen
	@./$(TARGET) bench | grep "info nodes"
	@touch *.[ch]pp
	$(MAKE) profuse
	@./$(TARGET) bench | grep "info nodes"
	@./$(TARGET) bench | grep "info nodes"
	@./$(TARGET) bench | grep "info nodes"

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(TARGET) ${OBJECTS:.o=.gcda}

-include $(DEPENDS)
