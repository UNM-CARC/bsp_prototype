#include "library.h"
#include <mpi.h>

#include <math.h>
#include <stdio.h>

void * initLAMMPS(int i);

void * setupLAMMPS(double a, double b);

void runLAMMPS(void *& lammps);

void * resetLAMMPS(void *& lammps, int i);

void deleteLAMMPS(void *& lammps, int i);
