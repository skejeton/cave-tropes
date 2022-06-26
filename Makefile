CXX=clang++
AR=llvm-ar
SHDC=./lib/sokol-tools-bin/bin/linux/sokol-shdc

SHARED_CFLAGS=-O0 --std=c++17

SRC_CFLAGS=-I. -MD
SRC_LIBS=-lGL -lXi -lXcursor -lasound -ldl -lm -lpthread -lX11 bin/imgui.a
SRC_UNITS=$(shell echo src/**.cpp)
SRC_HEADERS=$(shell echo src/**.h src/**.inl)
SRC_OBJECTS=$(SRC_UNITS:.cpp=.o)
SRC_DEPS=$(SRC_OBJECTS:.o=.d)

all: bin/cave_tropes

run: all
	bin/cave_tropes

bin/imgui.a:
	$(CXX) $(SHARED_CFLAGS) lib/imgui/*.cpp -c 
	$(AR) -rs bin/imgui.a *.o
	rm *.o

res/shaders.glsl:
	$(SHDC) -i res/shaders.glsl -o bin/shaders.h --slang glsl330

%.o: %.cpp
	$(CXX) $(SHARED_CFLAGS) $(SRC_CFLAGS) -c $< -o $@

bin/cave_tropes: bin/imgui.a res/shaders.glsl $(SRC_OBJECTS) Makefile
	$(CXX) $(SHARED_CFLAGS) $(SRC_CFLAGS) $(SRC_OBJECTS) $(SRC_LIBS) -o bin/cave_tropes

clean:
	rm -f bin/cave_tropes
	rm -f $(SRC_OBJECTS)
	rm -f $(SRC_DEPS)
	rm -f bin/imgui.a

-include $(SRC_DEPS)
