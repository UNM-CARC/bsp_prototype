#define LAMMPS_LIB_MPI

#include "library.h"
#include <mpi.h>

#include <math.h>
#include <stdio.h>

void * initLAMMPS();

void * setupLAMMPS(double a, double b);

void runLAMMPS(void *& lammps);

void * resetLAMMPS(void *& lammps);

void deleteLAMMPS(void *& lammps);
