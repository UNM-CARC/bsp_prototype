#!/bin/bash
set -x

module load gsl-2.5-gcc-7.3.0-i7icadp
module load openmpi-3.1.3-gcc-7.3.0-6javta3
module load cmake-3.15.4-gcc-7.3.0-uhxlwfq

git clone https://github.com/lammps/lammps.git

cd lammps

rm -rf build
mkdir -p build

cd build

cmake ../cmake
cmake --build .

cd ../..

mkdir -p lib
mkdir -p include
mkdir -p include/lammps
mkdir -p include/lammps/styles

mv lammps/build/liblammps.a lib/liblammps.a
mv lammps/build/liblammps_mpi_stubs.a lib/liblammps_mpi_stubs.a

cp lammps/src/* include/lammps
cp lammps/build/styles/* include/lammps/styles

mv lammps lib
