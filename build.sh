#!/bin/bash

if [ ! -d "build" ]; then
    mkdir build
fi

g++ main.cpp -std=c++11 -o build/sff
