

# Make sure the compiler can find include files from our Hello library.
include_directories (${MUIRRTI_SOURCE_DIR} ${HDF5_INCLUDE_DIR} ${GD_SOURCE_DIR} ${FFTW_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${GLUT_INCLUDE_DIR})

# Make sure the linker can find the Hello library once it is built.
link_directories (${MUIRRTI_BINARY_DIR} ${GD_BINARY_DIR} ${Boost_LIBRARY_DIRS})

add_executable (muir-rti muir-rti.cpp muir-data.cpp muir-hd5.cpp muir-constants.cpp muir-utility.cpp)
add_executable (muir-gl  muir-gl.cpp  muir-data.cpp muir-hd5.cpp muir-constants.cpp muir-utility.cpp muir-gl-shader.cpp muir-gl-data.cpp)

# Link the executable to the Hello library.
target_link_libraries (muir-rti ${HDF5_LIBRARIES_CXX} ${HDF5_LIBRARIES} ${GD_LIBRARY} ${FFTW_LIBRARY} ${FFTW_THREAD_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} gd)
target_link_libraries (muir-gl ${HDF5_LIBRARIES_CXX} ${HDF5_LIBRARIES} ${GD_LIBRARY} ${FFTW_LIBRARY} ${FFTW_THREAD_LIBRARY} ${GLUT_glut_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_DATA_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} gd)

