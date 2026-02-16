#!/usr/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

set -euo pipefail

echo "Installing required packages"
sudo apt-get update -y
sudo apt-get install -y cmake pkg-config libyaml-dev libsystemd-dev

echo "Installing optional packages (fasttext)"
sudo apt-get install -y fasttext libfasttext-dev || {
    echo "Warning: optional fasttext packages could not be installed."
}

echo "Running CMake configure"
mkdir -p build
cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_CLASSIFIER=ON -DCMAKE_INSTALL_PREFIX=/
cmake --build .
