#include "lammps_runner.h"

LAMMPS * initLAMMPS() {
    char *myargv[] = {strdup("lammps"), NULL};
    int myargc = sizeof(myargv) / sizeof(char*) - 1;

    const char *fname = "lj_setup.txt";

    LAMMPS *lammps = new LAMMPS(myargc, myargv, MPI_COMM_WORLD);
    lammps->input->file(fname);

    return lammps;
}

LAMMPS * setupLAMMPS(double a, double b) {
    printf("Setting Up Lammps\n\n");
    printf("a: %.4f\tb: %.4f\n", a, b);

    
    int lattices_dim = (int)ceil(cbrt(a / 4));

    FILE *fsetup;

    fsetup = fopen("lj_setup.txt", "w");
    fprintf(fsetup, "variable\tx index 1\n");
    fprintf(fsetup, "variable\ty index 1\n");
    fprintf(fsetup, "variable\tz index 1\n");
    fprintf(fsetup, "variable\txx equal %d*$x\n", lattices_dim);
    fprintf(fsetup, "variable\tyy equal %d*$y\n", lattices_dim);
    fprintf(fsetup, "variable\tzz equal %d*$z\n", lattices_dim);

    fprintf(fsetup, "units\tlj\n");
    fprintf(fsetup, "atom_style\tatomic\n");

    fprintf(fsetup, "lattice\tfcc 0.8442\n");
    fprintf(fsetup, "region\tbox block 0 ${xx} 0 ${yy} 0 ${zz}\n");
    fprintf(fsetup, "create_box\t1 box\n");
    fprintf(fsetup, "create_atoms\t1 box\n");
    fprintf(fsetup, "mass\t1 1.0\n");

    fprintf(fsetup, "velocity\tall create 1.44 87287 loop geom\n");

    fprintf(fsetup, "pair_style\tlj/cut 2.5\n");
    fprintf(fsetup, "pair_coeff\t1 1 1.0 1.0 2.5\n");

    fprintf(fsetup, "neighbor\t0.3 bin\n");
    fprintf(fsetup, "neigh_modify\tdelay 0 every 20 check no\n");

    fprintf(fsetup, "fix\t1 all nve\n");

    fclose(fsetup);

    FILE *frun;

    frun = fopen("lj_run.txt", "w");
    fprintf(frun, "run\t%d", int(b));

    fclose(frun);

    return initLAMMPS();
}

void runLAMMPS(LAMMPS *& lammps) {
    printf("Running Lammps iterations\n\n");

    const char *fname = "lj_run.txt";

    lammps->input->file(fname);

}

LAMMPS * resetLAMMPS(LAMMPS *& lammps) {
    delete lammps;
    return initLAMMPS();
}
