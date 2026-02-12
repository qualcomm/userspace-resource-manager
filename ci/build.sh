#!/usr/bin/bash

mkdir -p build
cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_CLASSIFIER=ON -DCMAKE_INSTALL_PREFIX=/
cmake --build .
