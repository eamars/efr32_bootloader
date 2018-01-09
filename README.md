# Hatch Bootloader for EFR32 Series MCUs

This repository contains source code for bootloader for EFR32 series MCU. For more information, please refer to `README.md` in bootloader folder. 

Dependencies
------------
You might need to install the following packages to build the firmware:
 - gcc-arm-none-eabi
 - cmake

If you want to program and debug the hatch, you will need to install following packages:
 - gdb-arm-none-eabi
 - jlink

Further more, if you would like to build documentations, you will need to install following packages:
 - doxygen

Optionally, `cppcheck`, a static code analyzer is helpful finding potential bugs or defects without programming the hatch.
 - cppcheck
 
Branches
--------
This repository contains three branches:
- stable: protected from pushing, can only be merge from development branch. Need to pass all tests.
- development: protected from pushing, can only be merge from development branch. Need to pass all unit tests.
- experimental: allow pushing or merging from other branches.

Tools
-----
There are several tools available when building or debugging the hatch:
- build_debug.sh: generate build scripts with debug symbols enabled (-O1).
- cleanup.sh: remove all CMake cache files and folders (need to run `build_debug.sh` for the next build).
- openocd_start.sh: start the OpenOCD with busbuster configuration loaded.
- program.sh: load program onto the hatch.
- gdb.sh: remotely debug the device using gdb. With `reset` parameter when executing the script, the program on hatch will be restarted and break at the start of `main()`.

Continuous Integration
----------------------
For building and testing devices, especially done by Jenkins, you might need to add Jenkins' account to both `dialout` and `plugdev` user group, allowing USB and Serial devices to be accessable.

sudo adduser jenkins dialout
sudo adduser jenkins plugdev

