#!/bin/bash
echo "Compiling..."
g++ -std=c++20 -I./src src/app/main.cpp src/dsp/*.cpp src/analysis/*.cpp src/utils/*.cpp -o build/crusher
echo "Done. Executable in build/crusher"
