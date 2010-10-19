
cmake_minimum_required (VERSION 2.6)
project (MUIR)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake.local)

FIND_PACKAGE(HDF5)
FIND_PACKAGE(Boost)

SUBDIRS(gd-2.0.35)

SET( CMAKE_CXX_FLAGS "-ggdb -Wall -Wextra -Weffc++ -fopenmp")

# Make sure the compiler can find include files from our Hello library.
include_directories (${MUIR_SOURCE_DIR} ${HDF5_INCLUDE_DIR} ${GD_SOURCE_DIR})

# Make sure the linker can find the Hello library once it is built.
link_directories (${MUIR_BINARY_DIR} ${GD_BINARY_DIR})

add_executable (readdata readdata.cpp muir-data.cpp)

# Link the executable to the Hello library.
target_link_libraries (readdata ${HDF5_LIBRARIES_CXX} ${HDF5_LIBRARIES} ${GD_LIBRARY} gd) 
