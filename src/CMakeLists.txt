
# Include file locations
include_directories (
 ${MUIRRTI_SOURCE_DIR}
 ${MUIRRTI_SOURCE_DIR}/include
 ${MUIRRTI_BINARY_DIR}
 ${CMAKE_CURRENT_BINARY_DIR}
 ${HDF5_INCLUDE_DIR}
 ${GD_SOURCE_DIR}
 ${FFTW_INCLUDE_DIR}
 ${Boost_INCLUDE_DIR}
 ${GLUT_INCLUDE_DIR}
 ${OPENGL_INCLUDE_DIR}
)

# Library locations
link_directories (
 ${MUIRRTI_BINARY_DIR}
 ${GD_BINARY_DIR}
 ${Boost_LIBRARY_DIRS}
)

SET(CORE_MUIR_LIBRARIES 
 ${HDF5_LIBRARIES_CXX}
 ${HDF5_LIBRARIES}
 ${FFTWF_LIBRARY}
 ${FFTWF_THREAD_LIBRARY}
 ${Boost_FILESYSTEM_LIBRARY}
 ${Boost_DATE_TIME_LIBRARY}
 ${Boost_SYSTEM_LIBRARY}
 ${Boost_THREAD_LIBRARY}
 ${ZLIB_LIBRARY}
 ${OPENCL_LIBRARIES}
)

# MUIR Common Library
add_library(muir STATIC
 muir-data.cpp
 muir-global.cpp
 muir-hd5.cpp
 muir-constants.cpp
 muir-utility.cpp
 muir-process.cpp
 muir-process-cl.cpp 
 muir-process-cpu.cpp
 muir-timer.cpp
)

# Create include files from GLSL and CL source
set(TXT_SOURCES
 stage1-phasecode.cl
 stage2-fft.cl
 stage3-power.cl
 stage4-findpeak.cl
 colorizer.frag
)

IF(XXD_VERSION)
# Turn GLSL shaders and CL kernels into include files
foreach(f ${TXT_SOURCES})
    message("Converting ${f} to ${f}.h")
execute_process( COMMAND "xxd" "-i" ${f} ${CMAKE_CURRENT_BINARY_DIR}/${f}.h
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#file(READ ${CMAKE_CURRENT_SOURCE_DIR}/${f} g HEX)
#string(REPLACE "\n"
#         "\\\\\n" g
#         ${g})
#message(${g}) 

endforeach(f)
ENDIF(XXD_VERSION)


# Create and link the executables
add_executable (muir-decode muir-decode.cpp)
target_link_libraries (muir-decode ${LINK_STATIC} muir ${CORE_MUIR_LIBRARIES})

add_executable (muir-rti muir-rti.cpp muir-plot.cpp)
target_link_libraries (muir-rti muir ${CORE_MUIR_LIBRARIES} ${GD_LIBRARY} gd)

add_executable (muir-gl  muir-gl.cpp  muir-gl-shader.cpp muir-gl-data.cpp)
target_link_libraries (muir-gl muir ${CORE_MUIR_LIBRARIES} ${GD_LIBRARY} ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARY} gd)

#add_executable (muir-cl  muir-cl.cpp)
#target_link_libraries (muir-cl muir ${CORE_MUIR_LIBRARIES} ${GD_LIBRARY} ${OPENCL_LIBRARIES} gd)
