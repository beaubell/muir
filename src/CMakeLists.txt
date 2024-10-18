

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

target_link_libraries(muir
  PUBLIC
    Threads::Threads
    hdf5::hdf5
    hdf5::hdf5_cpp
    FFTW::Float
    FFTW::FloatThreads
    PkgConfig::GD
    OpenCL::OpenCL
    Microsoft.GSL::GSL
    Boost::boost
    Boost::system
    Boost::filesystem
    Boost::thread
    OpenMP::OpenMP_CXX
)

# Include file locations
target_include_directories (muir
  PUBLIC
    ${MUIRRTI_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GSL_SOURCE_DIR}/include
)

# MUIR Validate Library
add_library(muir-val
 muir-validate-lib.cpp
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
target_link_libraries (muir-decode ${LINK_STATIC} muir)

add_executable (muir-validate muir-validate.cpp)
target_link_libraries (muir-validate ${LINK_STATIC} muir muir-val)

add_executable (muir-compare muir-compare.cpp)
target_link_libraries (muir-compare ${LINK_STATIC} muir muir-val)

add_executable (muir-rti muir-rti.cpp muir-plot.cpp)
target_link_libraries (muir-rti muir gd)

add_executable (muir-gl  muir-gl.cpp  muir-gl-shader.cpp muir-gl-data.cpp)
target_link_libraries (muir-gl muir gd ${GLUT_LIBRARIES} OpenGL::GL OpenGL::GLX X11::X11 GL)

