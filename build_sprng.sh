#!/bin/bash -l
module load openmpi-3.1.4-gcc-4.8.5-2e57mrc
configure --with-mpi --enable-debug && make && make install
