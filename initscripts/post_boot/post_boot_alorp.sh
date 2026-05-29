#!/bin/sh
# Copyright (c) 2026-2027, Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

echo 128 > /proc/sys/kernel/sched_util_clamp_min_rt_default

# Disable periodic kcompactd wakeups. We do not use THP, so having many
# huge pages is not as necessary.
echo 0 > /proc/sys/vm/compaction_proactiveness
