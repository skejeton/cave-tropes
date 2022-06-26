CXX?=clang++
AR?=llvm-ar
SHDC?=./lib/sokol-tools-bin/bin/linux/sokol-shdc
OUTPUT?=bin/cave_tropes
IMGUI_OUTPUT?=bin/imgui.a
SYS_LIBS?=-lGL -lXi -lXcursor -lasound -ldl -lm -lpthread -lX11

SHARED_CFLAGS=-O0 --std=c++17

SRC_CFLAGS=-I. -MD
SRC_LIBS=$(SYS_LIBS) $(IMGUI_OUTPUT)
SRC_UNITS=$(wildcard src/**.cpp)
SRC_HEADERS=$(wildcard src/**.h src/**.inl)
SRC_OBJECTS=$(SRC_UNITS:.cpp=.o)
SRC_DEPS=$(SRC_OBJECTS:.o=.d)

all: $(OUTPUT)

run: all
	$(OUTPUT)

$(IMGUI_OUTPUT):
	$(CXX) $(SHARED_CFLAGS) lib/imgui/*.cpp -c 
	$(AR) rcs -o $(IMGUI_OUTPUT) *.o
	rm *.o

bin/shaders.h: res/shaders.glsl
	$(SHDC) -i res/shaders.glsl -o bin/shaders.h --slang glsl330

%.o: %.cpp
	$(CXX) $(SHARED_CFLAGS) $(SRC_CFLAGS) -c $< -o $@

$(OUTPUT): $(IMGUI_OUTPUT) bin/shaders.h $(SRC_OBJECTS)
	$(CXX) $(SHARED_CFLAGS) $(SRC_CFLAGS) $(SRC_OBJECTS) $(SRC_LIBS) -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)
	rm -f bin/shaders.h
	rm -f $(IMGUI_OUTPUT)
	rm -f $(SRC_OBJECTS)
	rm -f $(SRC_DEPS)

include $(wildcard $(SRC_DEPS))