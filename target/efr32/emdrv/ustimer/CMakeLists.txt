MESSAGE("ENTER ${CMAKE_CURRENT_SOURCE_DIR}")

# get module name
GET_FILENAME_COMPONENT(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Collect all source file
FILE (GLOB SRC src/*.c)

# build library
ADD_LIBRARY (${LIB_NAME} ${SRC})

# add dependency for linking
ADD_DEPENDENCIES(${LIB_NAME} emlib)

# link libraries with top level library
TARGET_LINK_LIBRARIES(${LIB_NAME} emlib)
