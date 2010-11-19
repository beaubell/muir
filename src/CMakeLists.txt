

# Make sure the compiler can find include files from our Hello library.
include_directories (${MUIR_SOURCE_DIR} ${HDF5_INCLUDE_DIR} ${GD_SOURCE_DIR} ${FFTW_INCLUDE_DIR} ${Boost_INCLUDE_DIR})

# Make sure the linker can find the Hello library once it is built.
link_directories (${MUIR_BINARY_DIR} ${GD_BINARY_DIR} ${Boost_LIBRARY_DIRS})

add_executable (readdata readdata.cpp muir-data.cpp muir-hd5.cpp muir-constants.cpp muir-utility.cpp)
add_executable (muir-gl  muir-gl.cpp  muir-data.cpp muir-hd5.cpp muir-constants.cpp muir-utility.cpp)

# Link the executable to the Hello library.
target_link_libraries (readdata ${HDF5_LIBRARIES_CXX} ${HDF5_LIBRARIES} ${GD_LIBRARY} ${FFTW_LIBRARY} ${FFTW_THREAD_LIBRARY} -lboost_filesystem -lboost_date_time gd)
target_link_libraries (muir-gl ${HDF5_LIBRARIES_CXX} ${HDF5_LIBRARIES} ${GD_LIBRARY} ${FFTW_LIBRARY} ${FFTW_THREAD_LIBRARY} ${GLUT_glut_LIBRARY} -lboost_filesystem -lboost_date_time gd)

