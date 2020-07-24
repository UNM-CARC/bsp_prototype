#include "hpcg_runner.h"

void setupSPMV(double a, SparseMatrix & A, Vector & b, Vector & x, Vector & xexact) {
    // printf("Setting up HPCG\n");
    // printf("The value of a is: %.4f\n", a);

    int size = 1; 
    int rank = 0;
    int numThreads = 1;
    int pz = 0;
    int zl = 0;
    int zu = 0;

    int npx = 0;
    int npy = 0;
    int npz = 0;

    local_int_t nx,ny,nz;
    nx = (local_int_t) a;
    ny = (local_int_t) a;
    nz = (local_int_t) a;

    // Construct the geometry and linear system
    Geometry * geom = new Geometry;
    GenerateGeometry(size, rank, numThreads, pz, zl, zu, nx, ny, nz, npx, npy, npz, geom);
    
    printf("Size: %d\tRank: %d\tNum Threads: %d\tpz: %d\tzl: %d\tzu: %d\tnx: %d\tny: %d\tnz: %d\tnpx: %d\tnpy: %d\tnpz: %d\n", size, rank, numThreads, pz, zl, zu, nx, ny, nz, npx, npy, npz);

    InitializeSparseMatrix(A, geom);

    printf("Number of Rows: %d\tNumber of Columns: %d\tTotal Number of Non-Zeros: %lld\n", A.localNumberOfRows, A.localNumberOfColumns, A.totalNumberOfNonzeros);

    GenerateProblem(A, &b, &x, &xexact);
    SetupHalo(A);

    // printf("Number of Rows: %d\tNumber of Columns: %d\tTotal Number of Non-Zeros: %lld\n", A.localNumberOfRows, A.localNumberOfColumns, A.totalNumberOfNonzeros);

    FillRandomVector(x);
}

void runSPMV(SparseMatrix & A, Vector & x, Vector & b) {
    // printf("Running HPCG\n");
    // printf("Number of Rows: %d\tNumber of Columns: %d\tTotal Number of Non-Zeros: %lld\n", A.localNumberOfRows, A.localNumberOfColumns, A.totalNumberOfNonzeros);

    int ierr = ComputeSPMV_ref(A, x, b); // b = A*x

    if (ierr) printf("Error in call to SpMV: %d\n", ierr);
}

void setupHPCG(double a, SparseMatrix & A, Vector & b, Vector & x, Vector & xexact, CGData & data){
    
    int size, rank; 
    MPI_Comm_size(MPI_COMM_WORLD, & size);   
    MPI_Comm_rank(MPI_COMM_WORLD, & rank);  
    int numThreads = 1;
    int pz = 0;
    int zl = 0;
    int zu = 0;

    int npx = 0;
    int npy = 0;
    int npz = 0;

    local_int_t nx,ny,nz;
    nx = (local_int_t) a;
    ny = (local_int_t) a;
    nz = (local_int_t) a;

    // Construct the geometry and linear system
    Geometry * geom = new Geometry;
    GenerateGeometry(size, rank, numThreads, pz, zl, zu, nx, ny, nz, npx, npy, npz, geom);
     
    printf("Size: %d\tRank: %d\tNum Threads: %d\tpz: %d\tzl: %d\tzu: %d\tnx: %d\tny: %d\tnz: %d\tnpx: %d\tnpy: %d\tnpz: %d\n", size, rank, numThreads, pz, zl, zu, geom->nx, geom->ny, geom->nz, geom->npx, geom->npy, geom->npz);

    InitializeSparseMatrix(A, geom);

    printf("Number of Rows: %d\tNumber of Columns: %d\tTotal Number of Non-Zeros: %lld\n", A.localNumberOfRows, A.localNumberOfColumns, A.totalNumberOfNonzeros);

    GenerateProblem(A, &b, &x, &xexact);
    SetupHalo(A);

    // printf("Number of Rows: %d\tNumber of Columns: %d\tTotal Number of Non-Zeros: %lld\n", A.localNumberOfRows, A.localNumberOfColumns, A.totalNumberOfNonzeros);

    FillRandomVector(x);
    InitializeSparseCGData(A, data);   
   // printf("setupHPCG\n");

}

void runHPCG(SparseMatrix & A, Vector & x, Vector & b, double Maxiter, CGData & data){
   std::vector< double > opt_times(9,0.0);
   double tolerance = 0.0 ;
   int niter = 0;
   double normr = 0.0;
   double normr0 = 0.0;	
   bool doPreconditioning = true;
   CG_ref(A, data, b ,x , Maxiter, tolerance, niter, normr, normr0, & opt_times[0], doPreconditioning); 
   //printf("runHPCG\n");
}
