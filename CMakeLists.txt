
cmake_minimum_required (VERSION 2.6)
project (MUIR)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake.local)


#FINDGD()
FIND_PACKAGE(HDF5)
FIND_PACKAGE(Boost)
FIND_PACKAGE(PNG)
FIND_PACKAGE(GD)

# Make sure the compiler can find include files from our Hello library.
include_directories (${MUIR_SOURCE_DIR} ${HDF5_INCLUDE_DIR} ${PNG_PNG_INCLUDE_DIR})

# Make sure the linker can find the Hello library once it is built.
link_directories (${MUIR_BINARY_DIR})

add_executable (readdata readdata.cpp muir-data.cpp)

# Link the executable to the Hello library.
target_link_libraries (readdata ${HDF5_LIBRARIES_CXX} ${HDF5_LIBRARIES} ${PNG_LIBRARY} ${GD_LIBRARY}) 
