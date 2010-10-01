#!/bin/sh
./readdata /scratch/bellamy/20081024.001/d0000598.dt0.h5 0 1 > data.txt
graph -T X -x "-20 600" -y "-10 80" data.txt
#gdb ./readdata /scratch/bellamy/20081024.001/d0000598.dt0.h5 /Raw11/Data/Power/Data
