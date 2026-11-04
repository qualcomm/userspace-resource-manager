#!/bin/sh
# Copyright (c) 2026-2027, Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause

THP_PATH="/sys/kernel/mm/transparent_hugepage"

for policy in `ls -d /sys/devices/system/cpu/cpufreq/policy*`;
    do echo schedutil > $policy/scaling_governor;
done

echo 100 > /proc/sys/vm/swappiness

# 15GB threshold = 15360 MB
if [ "$RAM_MB" -lt 15360 ]; then
    echo "[URM] RAM < 15GB (${RAM_MB} MB). Disabling THP"
    [ -f "$THP_PATH/enabled" ] && echo never > "$THP_PATH/enabled"
fi

set_zram_mem_limit() {
    mem_limit_mb=$((RAM_MB / 2))

    for zram_dev in /sys/block/zram*; do
        [ -e "$zram_dev/initstate" ] || continue

        if [ "$(cat "$zram_dev/initstate")" = "1" ]; then
            echo "${mem_limit_mb}M" > "$zram_dev/mem_limit"
            echo "[URM][ZRAM] Set mem_limit=${mem_limit_mb}M for $(basename $zram_dev)"
            return
        fi
    done

    echo "[URM][ZRAM] ZRAM not ready, skipping mem_limit"
}

set_zram_mem_limit
