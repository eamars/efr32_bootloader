#!/usr/bin/env bash

# generate makefile
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DTARGET=efr32 -DBOARD="BOARD_HATCH_OUTDOOR" .
