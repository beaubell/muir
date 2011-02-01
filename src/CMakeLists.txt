
# Include file locations
include_directories (${MUIRRTI_SOURCE_DIR} ${HDF5_INCLUDE_DIR} ${GD_SOURCE_DIR} ${FFTW_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})

# Library locations
link_directories (${MUIRRTI_BINARY_DIR} ${GD_BINARY_DIR} ${Boost_LIBRARY_DIRS})

SET(CORE_MUIR_LIBRARIES ${HDF5_LIBRARIES_CXX} ${HDF5_LIBRARIES} ${FFTW_LIBRARY} ${FFTW_THREAD_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${ZLIB_LIBRARY})

# MUIR Common Library
add_library(muir STATIC muir-data.cpp muir-hd5.cpp muir-constants.cpp muir-utility.cpp)

# Create and link the executables
add_executable (muir-decode muir-decode.cpp)
target_link_libraries (muir-decode ${LINK_STATIC} muir ${CORE_MUIR_LIBRARIES})

add_executable (muir-rti muir-rti.cpp muir-plot.cpp)
target_link_libraries (muir-rti muir ${CORE_MUIR_LIBRARIES} ${GD_LIBRARY} gd)

add_executable (muir-gl  muir-gl.cpp  muir-gl-shader.cpp muir-gl-data.cpp)
target_link_libraries (muir-gl muir ${CORE_MUIR_LIBRARIES} ${GD_LIBRARY} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARY} gd)

