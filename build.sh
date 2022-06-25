set -e
alias shdc=./lib/sokol-tools-bin/bin/linux/sokol-shdc
shdc -i res/shaders.glsl -o bin/shaders.h --slang glsl330
clang++ -I. --std=c++17 -lGL -lXi -lXcursor -lasound -ldl -lm -lpthread -lX11 lib/imgui/*.cpp src/main.cpp -o bin/cave_tropes