#!/bin/sh
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause

get_machine() {
    DEVICE_NAME_PATH="/sys/devices/soc0/machine"
    TARGET_CONF_DIR="/etc/urm/target/"
    DT_COMPATIBLE="/proc/device-tree/compatible"

    name=""

    # Try /sys/devices/soc0/machine
    if [ -f "$DEVICE_NAME_PATH" ]; then
        read -r name < "$DEVICE_NAME_PATH"
        name=$(echo "$name" | tr '[:upper:]' '[:lower:]')
        if echo "$name" | grep -qE '^[[:alnum:]]+$'; then
            echo "$name"
            return 0
        fi
    fi

    # Fall back to /proc/device-tree/compatible.
    if [ -f "$DT_COMPATIBLE" ]; then
        name=$(tr '\0' '\n' < "$DT_COMPATIBLE" | while read -r token; do
            case "$token" in
                *,*)
                    candidate="${token#*,}"
                    candidate=$(echo "$candidate" | tr '[:upper:]' '[:lower:]')
                    if [ -e "${TARGET_CONF_DIR}${candidate}" ]; then
                        echo "$candidate"
                        break
                    fi
                    ;;
            esac
        done)
    fi

    echo "$name"
    return 0
}

get_ram_mb() {
    RAM_MB=0
    while read key value _; do
        [ "$key" = "MemTotal:" ] && break
    done < /proc/meminfo
    RAM_MB=$((value / 1024))
    export RAM_MB
}

check_and_execute() {
    script_path="$1"
    if [ -x "$script_path" ]; then
        "$script_path"
    fi
}

get_ram_mb

# The target-specific scripts are always installed alongside this dispatcher,
# so resolve their location relative to it rather than hardcoding a prefix.
POST_BOOT_DIR=$(dirname "$0")

# Try postboot common
script="$POST_BOOT_DIR/post_boot_common.sh"
check_and_execute "$script"

# Check if there exists any target-specific post boot script
machine=$(get_machine)
if [ -n "$machine" ]; then
    #below only needed for variants of the targets, if any
    case "$machine" in
        sa8775p)
            machine="qcs9100"
            ;;
        sa7255p|qcs8275)
            machine="qcs8300"
            ;;
        qcs6490)
            machine="qcm6490"
            ;;
        cq2390s|iq2390s)
            machine="cq2390m"
            ;;
        qcs8845)
            machine="alorp"
            ;;
        hamoa-iot-som|x1e80100)
            machine="hamoa"
            ;;
        purwa-iot-som|x1p42100)
            machine="purwa"
            ;;
        sa8797p|sa8797p-ride-embedded)
            machine="nord"
            ;;
        *)
            # Empty default case
            ;;
    esac

    # Try dynamic script resolution
    script="$POST_BOOT_DIR/post_boot_${machine}.sh"
    check_and_execute "$script"
fi
