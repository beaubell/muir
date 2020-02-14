# Introduction

This is the Modular Ionospheric UHF Radar (MUIR) data processing and visualization tool-set that was used to support the High-Frequency Active Aurora Research Program (HAARP) in Gakona, Alaska.  I developed it during my tenure at the Arctic Region Supercomputing Center (ARSC) as a student research projects assistant.

HAARP campaigns are a series of experiments spread over several weeks, during which gigabytes of data are collected from several sensors including the MUIR radar.  The data is then transferred to ARSC in Fairbanks, AK for processing.  The original program was MATLAB based and took over a hour to process one minute's worth of data.  Even parallelized, the resulting data took months to process.

The tool-set was created to speed up the processing by allowing it to run on the bare metal of High Performance Computers (HPC).  It uses C++, OpenMP, and OpenCL.  The result is something that can run in real-time on high performance workstations using General Purpose Graphics Processing Unit (GPGPU) hardware.  A minute's worth of data now takes 18 seconds to process on a single GPU and can be done on location as the data is collected.  A paper based on this work, can be accessed [here](https://ieeexplore.ieee.org/abstract/document/6031571). 

In addition to this, I built several tools for manipulating and visualizing the data at multiple stages.  The `muir-gl` tool can be pointed at a directory and it will load only the metadata for each file on startup. While panning around, each data file is loaded and unloaded as they go in and out of visibility on the fly.  The data's color palette range can be adjusted in realtime via OpenGL shaders in order to allow the observer to focus on the data that is important.

This time vs. range visualization shows the generation of artificial aurora as captured by the MUIR radar.

![Screenshot with overlay modules installed](/docs/screenshot_artificial_aurora.png?raw=true "Artificial Aurora")

Here is an example of the file explorer that pops on when moving the cursor to the right of the window.

![Screenshot showing 3D Stereo](/docs/screenshot_file_explorer.png?raw=true "File Explorer")

# Build Requirements

- Boost
- OpenGL
- OpenCL
- OpenAL
- GLEW
- GLUT
- HDF5
- FFTW
- libjpeg

If GUI components (OpenGL, X11, etc) are missing during build, only the commandline utilities will be built.

## on Fedora
`dnf install libxml2-devel libjpeg-devel gd-devel fftw-devel fftw-static glew-devel hdf5-devel ocl-icd-devel openal-devel glut-devel`

# Building
`mkdir build && cd build`

`cmake .. -D OpenGL_GL_PREFERENCE=GLVND`

