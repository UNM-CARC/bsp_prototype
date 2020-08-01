#include "lammps.h"
#include <mpi.h>
#include "input.h"

#include <math.h>
#include <stdio.h>

using namespace LAMMPS_NS;

LAMMPS * initLAMMPS();

LAMMPS * setupLAMMPS(double a, double b);

void runLAMMPS(LAMMPS *& lammps);

LAMMPS * resetLAMMPS(LAMMPS *& lammps);
