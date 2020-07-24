#ifndef HPCG_RUN_H
#define HPCG_RUN_H

#include "hpcg.hpp"

#include "CheckAspectRatio.hpp"
#include "GenerateGeometry.hpp"
#include "GenerateProblem.hpp"
#include "GenerateCoarseProblem.hpp"
#include "SetupHalo.hpp"
#include "CheckProblem.hpp"
#include "ExchangeHalo.hpp"
#include "OptimizeProblem.hpp"
#include "WriteProblem.hpp"
#include "ReportResults.hpp"
#include "mytimer.hpp"
#include "ComputeSPMV_ref.hpp"
#include "ComputeMG_ref.hpp"
#include "ComputeResidual.hpp"
#include "CG.hpp"
#include "CG_ref.hpp"
#include "Geometry.hpp"
#include "SparseMatrix.hpp"
#include "Vector.hpp"
#include "CGData.hpp"
#include "TestCG.hpp"
#include "TestSymmetry.hpp"
#include "TestNorms.hpp"

#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <vector>

void setupProblem(double a, SparseMatrix & A, Vector & b, Vector & x, Vector & xexact, bool distributed);

void setupSPMV( double a, SparseMatrix & A, Vector & b, Vector & x, Vector & xexact );
void runSPMV( SparseMatrix & A, Vector & x, Vector & b );

void setupHPCG(double a, SparseMatrix & A, Vector & b, Vector & x, Vector & xexact, CGData & data);
void runHPCG(SparseMatrix & A, Vector & x, Vector & b, double Maxiter, CGData & data);

#endif
