TARGET = noirsocks-cli

CXX = clang++
CXXFLAGS = --std=c++11 -march=native -O3 -pipe
LD_FLAGS = 

CORE_LIB = core/libnoirsocks_core.a

OBJ := $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))

RM = rm -rfv

LIBS = -L./core -lnoirsocks_core -lyaml-cpp -lcrypto -lboost_system -lpthread
INCS = -I./core/include

$(TARGET): $(CORE_LIB) $(OBJ)
	$(CXX) $(LD_FLAGS) -o $@ $(OBJ) $(LIBS)

$(CORE_LIB):
	@cd core; $(MAKE) all;

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCS) -c -o $@ $<

all: $(TARGET)

clean:
	@cd core; $(MAKE) clean;
	$(RM) $(TARGET) $(OBJ)
