MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file and pre-compiled static libraries
FILE (GLOB SRC *.c)
FILE (GLOB SLIBS archive/*.a archive/*.ar)

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC})

# link static libraries if any
TARGET_LINK_LIBRARIES(${LIB_NAME} ${SLIBS})
