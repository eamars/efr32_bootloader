#!/usr/bin/env bash
arm-none-eabi-gdb -batch -x target/efr32/scripts/program_openocd.gdb debug/WG_BOOTLOADER.out
