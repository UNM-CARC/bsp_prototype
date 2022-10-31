#!/bin/bash
set -x

module load gcc/11.2.0-otgt
module load gsl/2.7-gpnk
module load openmpi/3.1.6-t4zs
module load cmake/3.22.2-c2dw

git clone https://github.com/lammps/lammps.git

cd lammps

rm -rf build
mkdir -p build

cd build

cmake -DLAMMPS_LIB_MPI=ON ../cmake
cmake --build . -j

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
