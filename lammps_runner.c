#include "lammps_runner.h"

void * initLAMMPS(int i) {
    char logfile[64];
    
    if (i < 0) i = 0;
    sprintf(logfile, "lammps_output_%d.txt", i);

    char *myargv[] = {strdup("lammps"), strdup("-screen"), strdup(logfile),  NULL};
    int myargc = sizeof(myargv) / sizeof(char*) - 1;
    
    char *fname = strdup("lj_setup.txt");

    void * lammps = NULL;
    lammps_open(myargc, myargv, MPI_COMM_WORLD, &lammps);
    lammps_file(lammps, fname);
    
    return lammps;
}

void * setupLAMMPS(double a, double b) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("\nSetting Up Lammps\n");
        printf("Requesting %.0f Molecules for %.0f Time Steps\n", a, b);

        int lattices_dim = (int)ceil(cbrt(a / 4));

        printf("Actual Number of Molecules Allocated: %d\tnx: %d\tny: %d\tnz: %d\n", lattices_dim * lattices_dim * lattices_dim * 4, lattices_dim, lattices_dim, lattices_dim);

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
    }

    MPI_Barrier(MPI_COMM_WORLD);

    return initLAMMPS(0);
}

void runLAMMPS(void *& lammps) {
    char *fname = strdup("lj_run.txt");

    lammps_file(lammps, fname);

}

void * resetLAMMPS(void *& lammps, int i) {
    lammps_close(lammps);
    
    return initLAMMPS(i + 1);
}

void deleteLAMMPS(void *& lammps, int i) {
    lammps_close(lammps);
    
    char logfile[64];
    
    if (i < 0) i = 0;
    sprintf(logfile, "lammps_output_%d.txt", i);
    
    remove(logfile);
}
