# FFTW_INCLUDE_DIR = fftw3.h
# FFTW_LIBRARIES = libfftw3.a
# FFTW_FOUND = true if FFTW3 is found

IF(FFTW_HOME)
  FIND_PATH(FFTW_INCLUDE_DIR fftw3.h  ${FFTW_HOME}/include)
  FIND_LIBRARY(FFTW_LIBRARY libfftw3.a ${FFTW_HOME}/lib)
  FIND_LIBRARY(FFTW_THREAD_LIBRARY libfftw3_threads.a ${FFTW_HOME}/lib)
  FIND_LIBRARY(FFTWF_LIBRARY libfftw3f.a ${FFTW_HOME}/lib)
  FIND_LIBRARY(FFTWF_THREAD_LIBRARY libfftw3f_threads.a ${FFTW_HOME}/lib)
ELSE(FFTW_HOME)
  #  SET(TRIAL_PATHS
  #    $ENV{FFTW_HOME}/include
  #    /usr/include
  #    /usr/local/include
  #    /opt/include
  #    /usr/apps/include
  #  )
  #
  #  SET(TRIAL_LIBRARY_PATHS
  #    $ENV{FFTW_HOME}/lib
  #    /usr/lib 
  #    /usr/local/lib
  #    /opt/lib
  #    /sw/lib
  #    )
  #
  #  FIND_PATH(FFTW_INCLUDE_DIR fftw3.h ${TRIAL_PATHS})
  #  FIND_LIBRARY(FFTW_LIBRARY fftw3 ${TRIAL_LIBRARY_PATHS})
  FIND_PATH(FFTW_INCLUDE_DIR fftw3.h ${QMC_INCLUDE_PATHS})
  FIND_LIBRARY(FFTW_LIBRARY libfftw3.a ${QMC_LIBRARY_PATHS})
  FIND_LIBRARY(FFTW_THREAD_LIBRARY libfftw3_threads.a ${QMC_LIBRARY_PATHS})
  FIND_LIBRARY(FFTWF_LIBRARY libfftw3f.a ${QMC_LIBRARY_PATHS})
  FIND_LIBRARY(FFTWF_THREAD_LIBRARY libfftw3f_threads.a ${QMC_LIBRARY_PATHS})

ENDIF(FFTW_HOME)

SET(FFTW_FOUND FALSE)
IF(FFTW_INCLUDE_DIR AND FFTW_LIBRARY)
  MESSAGE(STATUS "FFTW_INCLUDE_DIR=${FFTW_INCLUDE_DIR}")
  MESSAGE(STATUS "FFTW_LIBRARY=${FFTW_LIBRARY}")
  MESSAGE(STATUS "FFTWF_LIBRARY=${FFTWF_LIBRARY}")
  SET(FFTW_FOUND TRUE)
ENDIF()

MARK_AS_ADVANCED(
   FFTW_INCLUDE_DIR
   FFTW_LIBRARIES
   FFTW_FOUND
)
