#!/usr/bin/env bash

# generate makefile
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DTARGET=efr32 -DVARIANT=EFR32MG12P432F1024GM48 .
