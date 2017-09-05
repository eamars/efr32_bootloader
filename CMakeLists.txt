# @file CMakeLists.txt
# @brief Top level CMake controller
# @author Ran Bao (ran.bao@wirelessguard.co.nz)
# @date March, 2017

MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)

# define project name
PROJECT("EFR32 Bootloader")

# include essential libraries
INCLUDE(CMakeForceCompiler)
INCLUDE(CTest)

# product
SET(HATCH_EXECUTABLE)
SET(ELF_EXTENSION "out")
SET(BINARY_EXTENSION "bin")
SET(HEX_EXTENSION "hex")
SET(SREC_EXTENSION "s37")

# add folder for modules
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# find compiler
INCLUDE("compiler")

# find git module
INCLUDE("GetGitRevisionDescription")

# ENABLE ASM
ENABLE_LANGUAGE(ASM)

SET(CMAKE_STATIC_LIBRARY_PREFIX)
SET(CMAKE_STATIC_LIBRARY_SUFFIX)

SET(CMAKE_EXECUTABLE_LIBRARY_PREFIX)
SET(CMAKE_EXECUTABLE_LIBRARY_SUFFIX)

# Set variables
SET (TARGET)
MESSAGE ("TARGET: ${TARGET}")
SET (BOARD)
MESSAGE("BOARD: ${BOARD}")

# target specific directory
SET(TARGET_ARCH)
SET(TARGET_CORE)

SET(BOARD_SPECIFIC_INCLUDE_DIRECTORY)
SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY)
SET(TARGET_ROOT_DIRECTORY)

SET(TARGET_ROOT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/${TARGET}")
SET(TARGET_SPECIFIC_INCLUDE_DIRECTORY "${CMAKE_SOURCE_DIR}/target/${TARGET}") # currently set use the same directory

SET(COMPILER_GENERIC_NAME "GCC")

# read version information from git
git_describe(VERSION --tags)

# parse version info
STRING(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
STRING(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
STRING(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
STRING(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-([0-9]+).*" "\\1" VERSION_BUILD "${VERSION}")
STRING(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+-(.*)" "\\1" VERSION_SHA1 "${VERSION}")
SET(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")


# build modules
ADD_SUBDIRECTORY(bootloader)

