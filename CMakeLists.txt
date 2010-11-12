
cmake_minimum_required (VERSION 2.6)
project (MUIR)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake.local)

FIND_PACKAGE(HDF5)
FIND_PACKAGE(Boost)
FIND_PACKAGE(PkgConfig)

SET(FFTW_HOME $ENV{FFTW_HOME})
FIND_PACKAGE(FFTW)

SUBDIRS(src gd-2.0.35)

#SET( CMAKE_CXX_FLAGS "-ggdb -Wall -Wextra -Weffc++ -fopenmp")
SET( CMAKE_CXX_FLAGS "-O3 -ggdb -Wall -Wextra -fopenmp")

