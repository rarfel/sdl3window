#!/usr/bin/env bash
cmake -DCMAKE_BUILD_TYPE=debug -DBUILD_SHARED_LIBS=OFF -S . -B debug && cmake --build debug && gdb ./debug/debug/sdl3Window
