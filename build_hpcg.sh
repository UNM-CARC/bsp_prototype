#!/bin/bash
set -x

module load gcc/11.2.0-otgt
module load gsl/2.7-gpnk
module load openmpi/3.1.6-t4zs

git clone git@github.com:JDTruj2018/HPCG_CDSE.git
mv HPCG_CDSE hpcg

cd hpcg
rm -rf build
mkdir -p build

cd build

../configure Wheeler_MPI_GCC
make -j

ar rvs libhpcg.a src/CG.o src/CG_ref.o src/CheckAspectRatio.o src/CheckProblem.o src/ComputeDotProduct.o src/ComputeDotProduct_ref.o src/ComputeMG.o src/ComputeMG_ref.o src/ComputeOptimalShapeXYZ.o src/ComputeProlongation_ref.o src/ComputeResidual.o src/ComputeRestriction_ref.o src/ComputeSPMV.o src/ComputeSPMV_ref.o src/ComputeSYMGS.o src/ComputeSYMGS_ref.o src/ComputeWAXPBY.o src/ComputeWAXPBY_ref.o src/ExchangeHalo.o src/finalize.o src/GenerateCoarseProblem.o src/GenerateGeometry.o src/GenerateProblem.o src/GenerateProblem_ref.o src/init.o src/MixedBaseCounter.o src/mytimer.o src/OptimizeProblem.o src/OutputFile.o src/ReadHpcgDat.o src/ReportResults.o src/SetupHalo.o src/SetupHalo_ref.o src/TestCG.o src/TestNorms.o src/TestSymmetry.o src/WriteProblem.o src/YAML_Doc.o src/YAML_Element.o

cd ../..

mkdir -p lib
mkdir -p include
mkdir -p include/hpcg

mv hpcg/build/libhpcg.a lib/libhpcg.a
cp hpcg/src/*.hpp include/hpcg

mkdir -p lib
mv hpcg lib
