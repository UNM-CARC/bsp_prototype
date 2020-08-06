#include "lammps_runner.h"

void * initLAMMPS() {
    // Simulate argc and argv - pass argument to disable output to screen
    char *myargv[] = {strdup("lammps"), strdup("-screen"), strdup("none"),  NULL};
    int myargc = sizeof(myargv) / sizeof(char*) - 1;
    
    char *fname = strdup("lj_setup.txt");

    // Initialize the Problem
    void * lammps = NULL;
    lammps_open(myargc, myargv, MPI_COMM_WORLD, &lammps);
    lammps_file(lammps, fname);
    
    return lammps;
}

void * setupLAMMPS(double a, double b) {
    int rank;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        printf("\nSetting Up Lammps\n");
        printf("On %d Ranks: Requesting %.0f Molecules per Rank for %.0f Time Steps\tTotal Requested number of Molecules: %d\n", size, a, b, size * (int)a);

        // Warning Statement: Not running on a cubic number of ranks
        if (cbrt(size) != round(pow(size, 1.0/3.0))) printf("WARNING: Not Running on a Cubic Number of Ranks\n");

        // Calculate grid dimensions based off of FCC layout (4 molecules per lattice cell)
        int lattices_dim = (int)ceil(cbrt((a * size) / 4));

        // Get new number of molecules and molecules per rank
        int mols_per_rank = lattices_dim * lattices_dim * lattices_dim * 4 / size;
        int mols_total = mols_per_rank * size;

        // Warning Statement: Had to adjust number of molecules to create perfect cube with FCC layout
        if (a != mols_per_rank) printf("WARNING: Number of Molecules Requested (a * comm_size) Does Not Result in Perfect Cube of Lattice Cells (nx = ny = nz = a * comm_size / 4) in FCC Layout (4 Molecules per Lattice Cell) - Adjusting Molecule Count Accordingly\n");

        printf("Actual Number of Molecules Allocated Per Rank: %d\tTotal Number of Molecules: %d\tnx: %d\tny: %d\tnz: %d\n", mols_per_rank, mols_total, lattices_dim, lattices_dim, lattices_dim);

        // Write setup commands into lj_setup.txt
        FILE *fsetup;

        fsetup = fopen("lj_setup.txt", "w");
        
        // Set Variables
        fprintf(fsetup, "variable\tx index 1\n");
        fprintf(fsetup, "variable\ty index 1\n");
        fprintf(fsetup, "variable\tz index 1\n");
        
        // Grid Size
        fprintf(fsetup, "variable\txx equal %d*$x\n", lattices_dim);
        fprintf(fsetup, "variable\tyy equal %d*$y\n", lattices_dim);
        fprintf(fsetup, "variable\tzz equal %d*$z\n", lattices_dim);

        fprintf(fsetup, "units\tlj\n");
        fprintf(fsetup, "atom_style\tatomic\n");

        // Create Box, Lattice Cells, and Atoms/Molecules
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

        // Wrtie run commands into lj_run.txt
        FILE *frun;

        frun = fopen("lj_run.txt", "w");
        fprintf(frun, "run\t%d", int(b));

        fclose(frun);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    return initLAMMPS();
}

// Run the Problem
void runLAMMPS(void *& lammps) {
    char *fname = strdup("lj_run.txt");
    lammps_file(lammps, fname);
}

// Reset the Problem (Delete and then Re-Initialize)
void * resetLAMMPS(void *& lammps) {
    deleteLAMMPS(lammps);   
    return initLAMMPS();
}

// Free Memory
void deleteLAMMPS(void *& lammps) {
    lammps_close(lammps); 
}
