
cmake_minimum_required (VERSION 2.6)
project (MUIRRTI)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake.local)
SET(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
#SET(CMAKE_EXE_LINKER_FLAGS "-static")

FIND_PACKAGE(HDF5)
SET(Boost_ADDITIONAL_VERSIONS "1.45.0 1.45")
SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost COMPONENTS date_time system filesystem)
FIND_PACKAGE(PkgConfig)
FIND_PACKAGE(OpenGL)
FIND_PACKAGE(GLUT)
FIND_PACKAGE(ZLIB)

SET(FFTW_HOME $ENV{FFTW_HOME})
FIND_PACKAGE(FFTW)

SUBDIRS(src gd-2.0.35)

#SET( CMAKE_CXX_FLAGS "-ggdb -Wall -Wextra -Weffc++ -fopenmp")
SET( CMAKE_CXX_FLAGS "-O3 -ggdb -Wall -Wextra -fopenmp")

