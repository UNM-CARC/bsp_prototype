#!/bin/bash -l
module load openmpi-3.1.4-gcc-4.8.5-chobv7u
./configure --with-mpi --with-fortran=no --prefix=/usr && make install
