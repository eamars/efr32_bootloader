#!/usr/bin/env bash

GDB_EXECUTABLE=arm-none-eabi-gdb
EXECUTABLE=debug/WG_BOOTLOADER.out

# If the first argument is "reset"
if [ "$1" = "reset" ]; then
    ${GDB_EXECUTABLE} -x target/efr32/scripts/debug_reset.gdb ${EXECUTABLE} -tui
    exit 1
fi

${GDB_EXECUTABLE} -x target/efr32/scripts/debug.gdb ${EXECUTABLE} -tui
