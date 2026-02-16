#!/usr/bin/bash

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

mkdir -p build
cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_CLASSIFIER=ON -DCMAKE_INSTALL_PREFIX=/
cmake --build .
