/*
 * Copyright 2019. Sahba Tashakkori, Keira Haskins and Patrick G. Bridges, University of New Mexico
 * Copyright 2015. Oscar Mondragon
 * bsp_prototype.c (previously appGen.c)
 * Description: 
 * For the stencil part, parts of the code are slight modification of Jeff Hammonds codebase 'PRK Stencil'
 * https://github.com/jeffhammond/PRK
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <openssl/ssl.h>
#include <math.h>
#include <getopt.h>

#include <gsl/gsl_rng.h>
#include "gsl-sprng.h"
#include <gsl/gsl_randist.h>

#ifdef __INTEL_COMPILER
#include <mkl.h>
#else
#include <gsl/gsl_cblas.h>
#endif

//#include "sprng_cpp.h"
#include <assert.h>

#include "mpi.h"

#include <time.h>
#include <sys/time.h>

#ifdef PATH_MAX
#define FNAMELEN (PATH_MAX + NAME_MAX)
#else
#define FNAMELEN (NAME_MAX + 3)
#endif

// Header for amq send and util functions
#include "amqp_producer.h"

// Header for HPCG function calls
#include "hpcg_runner.h"

// Header for LAMMPS function calls
#include "lammps_runner.h"

//Stencil Radius
#define RADIUS 1
#define SECOND_TO_MICRO_FACTOR 1000000
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Rabit constants
// The message size: 8 KB
#define RABBIT_MESSAGE_SIZE 2<<13
#define RABBIT_MESSAGE_COUNT 2
//the probability for making a rabbit send call
#define RABBIT_PROB 0.1
#define VERBOSE_RABBIT_SEND 0
static unsigned int makeRabbitCalls = 0;
static char rabbitIP[18]; // this will be read from command line (-r option)
#define RABBIT_PORT 5672 // this is rabbit broker's defualt port

static volatile int WORKLOAD_VALUE = 1;
static volatile int WORKLOAD_OP_VALUE = 1;
static volatile double FWQ_CALIBRATE = 1.0;

/*
 * If distribution is:
 * exponential,    a = interarrival mean
 * gaussian,     a = interarrival mean, b = interarrival stddev
 * flat,     a = start number, b = end number
 * pareto,    a = exponent (i.e., shape)
 *      b = scale (shift to the right ~ largest value in tail),
 * constant    a = interarrival constant duration, b does not have effect
 */

MPI_Comm my_comm = 0;
unsigned char DEBUG = 1;
struct coll_time{
    int rank;
    double start;
    double bstart;
    double bend;
    double workload_max;
};

enum bsp_workload {
    WORKLOAD_SLEEP = 0,
    WORKLOAD_DGEMM,
    WORKLOAD_STREAM,
    WORKLOAD_FBENCH,
    WORKLOAD_IO,
    WORKLOAD_FWQ,
    WORKLOAD_SPMV,
    WORKLOAD_HPCG,
    WORKLOAD_LAMMPS
};
enum bsp_workload workload = WORKLOAD_FWQ;
const char *workload_str = "fwq";

// a methods to exit in case of an error!
void err_out(const char* errMessage){
    if(errMessage){
        fprintf(stderr, "%s\n", errMessage);
    }
    MPI_Finalize();
}

/* 
 * Code to sleep for a pre-determined about of time on an x86_64 system as
 * closely as possible by busy-waitng on the time stamp counter 
 */
unsigned long long rdtsc(void)
{
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

void* myAlloc(size_t size)
{
    void* memory = malloc(size);
    if (memory == NULL){
        char buffer[50];
        sprintf(buffer, "Unable to allocate memory of size %lu", size);
        err_out(buffer);
    }
    return memory;
}

//dividing the cores into x and y
void divide_cores(int ranks, int* coresX, int* coresY)
{
    for (*coresX=(int) (sqrt(ranks+1.0)); *coresX>0; (*coresX)--) {
        if ((ranks%(*coresX)) == 0) {
            *coresY = ranks/(*coresX);
            break;
        }
    }
}

/*
 * get_clocks_per_nanosecond and sleep_rdtsc are based on code in
 * http://sci.tuomastonteri.fi/programming/cplus/x86timer
 */
double get_clocks_per_nanosecond() 
{
    uint64_t bench1=0;
    uint64_t bench2=0;
    double clocks_per_nanosecond=0;

    bench1=rdtsc();

    usleep(2500000); // 2.5 second

    bench2=rdtsc();

    clocks_per_nanosecond = 4.0e-10 * (double)(bench2 - bench1);

    return clocks_per_nanosecond;
}

void sleep_rdtsc(uint64_t nanoseconds, double clocks_per_nanosecond)
{
    uint64_t begin=rdtsc();
    uint64_t now;
    uint64_t dtime = (uint64_t)(((double) nanoseconds)*clocks_per_nanosecond);
    do {
        now=rdtsc();
    } while ( (now-begin) < dtime);
}


/* Code to generate the length of the random intervals on each node */
enum rng_type {
    RNG_ERROR=-1,
    RNG_GAUSSIAN,
    RNG_EXPONENTIAL,
    RNG_FLAT,
    RNG_PARETO,
    RNG_CONSTANT
};

enum rng_type init_rng_type(char *distribution)
{
    if (strcmp(distribution,"gaussian") == 0){
        return RNG_GAUSSIAN;
    } else if (strcmp(distribution,"exponential") == 0){
        return RNG_EXPONENTIAL;
    } else if (strcmp(distribution,"flat") == 0){
        return RNG_FLAT;
    } else if (strcmp(distribution,"pareto") == 0){
        return RNG_PARETO;
    } else if (strcmp(distribution, "constant") == 0)
        return RNG_CONSTANT;
    else return RNG_GAUSSIAN;
}

double generate_interval_rng(gsl_rng *r, enum rng_type rng_type, double a, double b) 
{
    double inter_time;
    double inter_mean, inter_stddev, exponent, scale; 
    do {
        switch (rng_type) {
            case RNG_GAUSSIAN:
                inter_mean = a;
                inter_stddev = b;
                inter_time =  gsl_ran_gaussian (r, inter_stddev) + inter_mean;
                break;
            case RNG_EXPONENTIAL:
                inter_mean = a;
                inter_time =  gsl_ran_exponential (r, inter_mean);
                break;
            case RNG_FLAT:
                inter_time = gsl_ran_flat (r,a,b);
                break;
            case RNG_PARETO:	
                exponent = a;
                scale = b;
                inter_time = gsl_ran_pareto (r,exponent,scale);
                break;
                /* Note the straight return out in the next two to avoid the
                 * while loop if a is 0.0 */
            case RNG_CONSTANT:
                return a;
            default:
                puts("No distribution specified - Using Gaussian as default");
                return 0.0;
        }
    } while (inter_time < 0.0);
    return inter_time;
}

unsigned long hash_string(char *str)
{
    unsigned long hash = 0;
    int c;

    while ((c = *str++) != 0)
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}
/*
 *
 void divideLeftOver(const int rank, const int cores, const int gridSize, const int id,  int* start, int* end){
 int width = gridSize / cores;
 int leftOver = gridSize % cores;
 if (id < leftOver){
    *start = id * (width + 1); 
    *end = *start + width;
 }else{
    *start = leftOver * (width + 1) + (id - leftOver) * width;
    *end = *start + width -1 ;
 }
 width = *end - *start + 1;
 if(width == 0){
    char errorMessage[30];
    sprintf(errorMessage, "rank %d has 0 work to do!", rank);
    err_out(errorMessage);
 }
 }
 */

//establish the connection to the broker. Return connection upon success, die otherwise!
amqp_connection_state_t getAmqpConnection(){
    amqp_connection_state_t conn;
    if (DEBUG){
        fprintf(stdout, "trying to connect to host %s port %d\n", rabbitIP, RABBIT_PORT);
    }
    if(setupAmq(&conn, rabbitIP, RABBIT_PORT)){
        fprintf(stderr, "could not open connection\n");
        MPI_Finalize();
        exit(-1);
    }
    if (DEBUG){
        fprintf(stdout, "Successfully openned connection to %s:%d\n", rabbitIP, RABBIT_PORT);
    }
    return conn;
}

/*
 * Set up communicator and directions for stencil communication
 */
int setup_stencil(int stencil_size, int *left, int *right, int *up, int *down, 
        double **left_buf, double **right_buf, double **up_buf, double **down_buf,
        double **values)
{
    int grid[2];
    int period[2] = {1, 1};
    int rank, nprocs;

    grid[0] = 0; grid[1] = 0;
    MPI_Comm_size(my_comm, &nprocs);
    MPI_Dims_create(nprocs, 2, grid);
    if (rank == 0 && DEBUG){
        printf("There are %d cores in x and %d cores in y\n", grid[0], grid[1]);
    }
    period[0] = 1; period[1] = 1;
    MPI_Cart_create(MPI_COMM_WORLD, 2, grid, period, 1, &my_comm);

    MPI_Comm_rank(my_comm, &rank);
    MPI_Comm_size(my_comm, &nprocs);

    MPI_Cart_shift(my_comm, 0, -1, &rank, left);
    MPI_Cart_shift(my_comm, 0, 1, &rank, right);
    MPI_Cart_shift(my_comm, 1, -1, &rank, down);
    MPI_Cart_shift(my_comm, 1, 1, &rank, up);

    *values = (double*) myAlloc(4 * stencil_size * sizeof(double));
    for(int i=0; i < 4*stencil_size; i++){
        (*values)[i] = double(rank);
    }
    *up_buf = (double*) myAlloc(stencil_size * 2 * sizeof(double));
    *down_buf = *up_buf + stencil_size;
    *left_buf = (double*) myAlloc(stencil_size * 2 * sizeof(double));
    *right_buf = *left_buf + stencil_size;

    return 0;
}

int tear_down_stencil(double *right, double *left, double *up, double *down, double *val)
{
    free(up);
    free(left);
    free(val);
    return 0;
}

enum rng_type rng_type = RNG_ERROR;

double *DGEMM_A, *DGEMM_B, *DGEMM_C;
int DGEMM_N, DGEMM_iter;

SparseMatrix HPCG_A;
Vector HPCG_b, HPCG_x, HPCG_xexact, HPCG_xorig;
CGData HPCG_data, HPCG_data_orig;
int HPCG_iter;

void *lammps;

struct io_params_s {
    size_t io_size = 1;
    FILE *handle;
    const char *fname;
    const char *pname;
    char *ary;
    const char *slash;
} io_params;

void fill(double *p, int n)
{
    for (int i = 0; i < n; ++i)
        p[i] = 2 * drand48() - 1;
}

void calibrate_fwq(int loops, int w, gsl_rng *r, double a, double b, double cpn) 
{
    double rate = 0.0;
    for (int i = - (int)(loops / 10); i < loops; i++) {
        WORKLOAD_OP_VALUE = 1.0;

        uint64_t begin = rdtsc();

        double inter_time = 0;
        inter_time = generate_interval_rng(r, rng_type, a, b);
        assert(inter_time >= 0.0);
        for (int j = 0; j < inter_time; j++) {
            WORKLOAD_OP_VALUE += j;
            asm("");                    
        }

        uint64_t end = rdtsc();

        if (i >= 0) rate += inter_time / ((double)(end - begin) / cpn / 1000.0);
    }

    FWQ_CALIBRATE = rate / (double)loops;
    printf("FWQ Calibrate: %.4f\n", FWQ_CALIBRATE);
}

void reset_workload(int w)
{
    switch(w) {
        case WORKLOAD_LAMMPS:
            lammps = resetLAMMPS(lammps);
            break;
        case WORKLOAD_HPCG:
            resetHPCG(HPCG_x, HPCG_xorig, HPCG_data, HPCG_data_orig);
            break;
        case WORKLOAD_SPMV:
            resetSPMV(HPCG_x, HPCG_b);
            break;
        default:
            break;
    }
}

int init_workload(int w, gsl_rng *r, char *distribution, double a, double b)
{
    double *buf;
    size_t realiosize;
    rng_type = init_rng_type(distribution);
    switch (w) {
        case WORKLOAD_LAMMPS:
            lammps = setupLAMMPS(a, b);
            break;
        case WORKLOAD_HPCG:
            setupHPCG(a, HPCG_A, HPCG_b, HPCG_x, HPCG_xexact, HPCG_data);
            HPCG_xorig = HPCG_x;
            HPCG_data_orig = HPCG_data; 
            HPCG_iter = (int)b;
            break;
        case WORKLOAD_SPMV:
            setupSPMV(a, HPCG_A, HPCG_b, HPCG_x, HPCG_xexact);
            HPCG_iter = (int)b;
            break;
        case WORKLOAD_FWQ:
            if (rng_type < 0) {
                return -1;
            }
            break;
        case WORKLOAD_SLEEP:
            if (rng_type < 0) {
                return -1;
            }
            break;
        case WORKLOAD_DGEMM:
            DGEMM_N = a;
            DGEMM_iter = b;
            buf = (double *)malloc(3 * (int)DGEMM_N * (int)DGEMM_N * sizeof(double));
            DGEMM_A = buf + 0;
            DGEMM_B = DGEMM_A + DGEMM_N * DGEMM_N;
            DGEMM_C = DGEMM_B + DGEMM_N * DGEMM_N;
            fill(DGEMM_A, DGEMM_N * DGEMM_N);
            fill(DGEMM_B, DGEMM_N * DGEMM_N);
            fill(DGEMM_C, DGEMM_N * DGEMM_N);
            break;
        case WORKLOAD_IO:
            realiosize = io_params.io_size * 1024;
            assert( (io_params.ary = (char*) malloc( realiosize ) ) != NULL );
            for( size_t i = 0; i < realiosize; i++ ) {
                io_params.ary[i] = rand();
            }
            io_params.fname = getenv( "BSPGEN_FILE_IO_NAME" );
            if (io_params.fname == NULL || strlen( io_params.fname ) == 0) {
                io_params.fname = "BSPGEN_FILE_IO";
            }

            io_params.pname = getenv( "BSPGEN_FILE_IO_PATH" );
            if (io_params.pname == NULL) {
                io_params.pname = "";
                io_params.slash = "";
            } else {
                /* Even if the pname ends in /, adding an extra / shouldn't 
                   hurt on any POSIX-flavored filesystem  */
                io_params.slash = "/";
            }
            break;
        default:
            assert(0 && "Unknown workload!");
    }
    return 0;
}

void cleanup_workload( int w, gsl_rng *r, char *distribution, double a, double b)
{
    switch(w) {
        case WORKLOAD_LAMMPS:
            deleteLAMMPS(lammps);
            break;
        case WORKLOAD_IO:
            free( io_params.ary );
            break;
        default:
            break;
    }
}


/* Do one iteration of whatever comoute workload was requested */
void run_workload(int w, gsl_rng *r, double a, double b, double cpn)
{
    double inter_time = 0;
    switch(w) {
        case WORKLOAD_LAMMPS:
            runLAMMPS(lammps);
            break;
        case WORKLOAD_HPCG:
            runHPCG(HPCG_A, HPCG_x, HPCG_b, HPCG_data, HPCG_iter);
            break;
        case WORKLOAD_SPMV:
            for ( int i = 0; i < HPCG_iter; i++ ) {
                runSPMV(HPCG_A, HPCG_x, HPCG_b);
            }
            break;
        case WORKLOAD_FWQ:
            inter_time = generate_interval_rng(r, rng_type, a, b);
            assert(inter_time >= 0.0);
            for (long long int i = 0; i < (long long int)((long long int)FWQ_CALIBRATE * (long long int)inter_time); i++) {
                WORKLOAD_VALUE += i;
                asm("");
            }
            break;
        case WORKLOAD_SLEEP:
            inter_time = generate_interval_rng(r, rng_type, a, b);
            assert(inter_time >= 0.0 );
            if (inter_time > 0){
                sleep_rdtsc(1000  * inter_time, cpn);
            }
            break;
        case WORKLOAD_DGEMM:
            for (int i = 0; i < DGEMM_iter; i++) {
                cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, 
                        DGEMM_N, DGEMM_N, DGEMM_N, 1.0,
                        (double *)DGEMM_A, DGEMM_N, (double *)DGEMM_B,
                        DGEMM_N, 1.0, (double *)DGEMM_C, DGEMM_N);
            }
            break;
        case WORKLOAD_IO:
            {
                int my_rank;
                static char buf[FNAMELEN];
                MPI_Comm_rank( my_comm, &my_rank );

                snprintf( buf, FNAMELEN, "%s%s%s.%d", io_params.pname, io_params.slash, io_params.fname, my_rank );
                io_params.handle = fopen( buf, "w" );
                fwrite( io_params.ary, sizeof(char), io_params.io_size * 1024, io_params.handle );
                fclose( io_params.handle );
            }
            break;
        default:
            assert(0 && "Unknown workload!");
    }
}

/* 
 * Main loop for the program - sleep in a barrier some number of iterations
 * with the length of each iteration drawn from a random distribution. 
 * Log the desired local, actual local, and actual global sleep times
 * as we run for later output.
 * Also if stencil_size > 0, each node halos stencil_size doubles with its neighbors
 * in asnearly a square topology as we can find.
 */

int barrier_loop(double a, double b, char * distribution, int stencil_size, int innerloop_itr, int iterations, 
        struct coll_time * times_buffer, double cpn, gsl_rng *r, amqp_connection_state_t conn)
{
    double coll_start = 0.0;
    double coll_bstart = 0.0;
    double coll_bend = 0.0;
    double rank_start_time = 0.0;
    int i,j;
    int rank = 0;
    int nprocs = 0;
    MPI_Request requests[8];
    int left_rank, right_rank, up_rank, down_rank;
    double *left_buf, *right_buf, *up_buf, *down_buf, *values;
    char *rabbit_message;

    for (i = 0; i < 8; i++)
        requests[i] = MPI_REQUEST_NULL;

    if (init_workload(workload, r, distribution, a, b)) {
        fprintf(stderr, "ERROR: could not initialize the workload!");
        return -1;
    }

    // Do we need to calibrate FWQ workload?
    if (workload == WORKLOAD_FWQ) {
        int loops = iterations;
        calibrate_fwq(std::max(loops, 1000), workload, r, a, b, cpn);
    }

    //initializing the rabbit message
    if(makeRabbitCalls){
        rabbit_message = (char*) myAlloc(RABBIT_MESSAGE_SIZE);
        int i;
        for (i = 0; i < RABBIT_MESSAGE_SIZE ; i++) {
            rabbit_message[i] = 'a';
        }
    }

    // If we're going to be doing a stencil, set up the cartesian communivator
    // we will use.
    if (stencil_size){
        setup_stencil(stencil_size, &left_rank, &right_rank, &up_rank, &down_rank,
                &left_buf, &right_buf, &up_buf, &down_buf, &values);
    } 

    MPI_Comm_rank(my_comm, &rank);
    MPI_Comm_size(my_comm, &nprocs);

    //this is not used directly it is just for fining differences
    rank_start_time = MPI_Wtime();

    /* We start at -5 to do 5 warmup iterations that are not recorded */
    for( i = -5; i < iterations; i++) {
        coll_start = MPI_Wtime();
        for(j=0; j<innerloop_itr; j++){
            if (stencil_size){
                //top bottom right left
                MPI_Irecv(up_buf, stencil_size, MPI_DOUBLE, up_rank, 811, my_comm, &requests[0]);
                MPI_Irecv(down_buf, stencil_size, MPI_DOUBLE, down_rank, 823, my_comm, &requests[1]);
                MPI_Irecv(right_buf, stencil_size, MPI_DOUBLE, right_rank, 919, my_comm, &requests[2]);
                MPI_Irecv(left_buf, stencil_size, MPI_DOUBLE, left_rank, 977, my_comm, &requests[3]);
            }

            //now whatever our computational workload is
            run_workload(workload, r, a, b, cpn);

            //Sahba: It's easy to separate the the timing of workload and stancil is that what we want though?
            //now sends
            if(stencil_size){
                MPI_Isend(values, stencil_size, MPI_DOUBLE, down_rank, 811, my_comm, &requests[4]);
                MPI_Isend(values + stencil_size, stencil_size, MPI_DOUBLE, up_rank, 823, my_comm, &requests[5]);
                MPI_Isend(values + 2*stencil_size, stencil_size, MPI_DOUBLE, left_rank, 919, my_comm, &requests[6]);
                MPI_Isend(values + 3*stencil_size, stencil_size, MPI_DOUBLE, right_rank, 977, my_comm, &requests[7]);
            }
            // with uniform probability send rabbit mesages
            // in case we do not have stencil but we still wanna do rabbit!
            if(makeRabbitCalls && gsl_rng_uniform(r) < RABBIT_PROB){
                //the '1' means produce verbose output
                sendBatch(conn, "test", RABBIT_MESSAGE_COUNT, RABBIT_MESSAGE_SIZE, rabbit_message, VERBOSE_RABBIT_SEND);
            }
            if (stencil_size) { 
                MPI_Waitall(8, requests, MPI_STATUSES_IGNORE);
            }
        }//end of inner loop

        coll_bstart = MPI_Wtime();
        MPI_Barrier(MPI_COMM_WORLD);
        coll_bend = MPI_Wtime();

        coll_start =     ( coll_start - rank_start_time ) * SECOND_TO_MICRO_FACTOR;
        coll_bstart =     ( coll_bstart - rank_start_time ) * SECOND_TO_MICRO_FACTOR;
        coll_bend   =     ( coll_bend   - rank_start_time ) * SECOND_TO_MICRO_FACTOR;

        /* Don't record warmup iterations  */
        if (i >= 0) {
            times_buffer[ i ].rank = rank;
            times_buffer[ i ].start = coll_start;
            times_buffer[ i ].bstart = coll_bstart;
            times_buffer[ i ].bend =  coll_bend;
        }

        reset_workload(workload);
    }// end of main loop
    //freeing the bufferes used for MPI exchange operations
    if(stencil_size){
        tear_down_stencil(right_buf, left_buf, up_buf, down_buf, values);
    }
    if(makeRabbitCalls){
        free(rabbit_message);
    }

    cleanup_workload(workload, r, distribution, a, b);

    return 0;
}

char experimentID[20];
void set_experiment_id(gsl_rng *r)
{
    const char *str = "0123456789abcdef";
    int root = 0;
    int length = 13;
    int rank;

    /* Share the experiment ID across ranks */
    MPI_Comm_rank(my_comm,&rank);
    if (rank == 0) {
        int i;
        // We use GSL here, so we'll get a fixed experiment ID for a 
        // fixed set of parameters, which is a good thing because it
        // lets us consistently regenerate experiments.
        for (i = 0; i < length; i++) {
            experimentID[i] = str[gsl_rng_uniform_int(r, 16)];
        }
        experimentID[i] = 0;
    }
    MPI_Bcast(&experimentID, sizeof(experimentID),
            MPI_CHAR, root, my_comm);
    return;
}


void write_buffer(double a, double b, char * distribution, int stencil_size, int iterations, int innerloop_itr,
        struct coll_time * times_buffer, gsl_rng *r, char *outfile)
{
    FILE *f_time;
    int rank, nproc;
    int i;

    MPI_Comm_rank(my_comm,&rank);
    MPI_Comm_size(my_comm,&nproc);

    /* Print the logged data to the local data file */
    f_time = fopen(outfile, "w");
    fprintf(f_time, "[\n");
    for (i = 0; i < iterations; i++) {
        fprintf( f_time, "{ " );
        fprintf( f_time, " \"uniq_id\": \"%s\", "
                " \"communicator\": %lu, "
                " \"comm_size\": %d, "
                " \"rank\": %d, "
                " \"workload\": \"%s\", "
                " \"distribution\": \"%s\", "
                " \"a\": %f, "
                " \"b\": %f, "
                " \"stencil_size\": %d, "
                " \"iterations\": %d, "
                " \"inner_loop_itr\": %d, ",
                experimentID, (unsigned long)my_comm, nproc, rank,
                workload_str, distribution, a, b, stencil_size,
                iterations, innerloop_itr);
        fprintf( f_time, 
                " \"iteration\": %d, "
                " \"work_start\": %.3lf, "
                " \"barrier_start\": %.3lf, "
                " \"barrier_end\": %.3lf, "
                " \"workload_usec\": %.3lf, "
                " \"workload_max_usec\": %.3lf, "
                " \"interval_max_usec\": %.3lf "
                " }",
                i, 
                times_buffer[i].start         ,
                times_buffer[i].bstart         ,
                times_buffer[i].bend           ,
                times_buffer[i].bstart - times_buffer[i].start,
                times_buffer[i].workload_max,
                times_buffer[i].bend - times_buffer[i].start );

        if (i + 1 < iterations) {
            fprintf(f_time, ",\n");
        } else {
            fprintf(f_time, "\n");
        }
    }
    fprintf(f_time, "]\n");
    fclose(f_time);
}

static char distribution[256] = "gaussian";
static double a = 100000, b = 10000;
static unsigned long iterations = 1000;
static unsigned long initseed = 0;
static unsigned int verbose = 0;


static struct option longargs[] =
{
    {"a", required_argument, 0, 'a'},
    {"b", required_argument, 0, 'b'},
    {"distribution", required_argument, 0, 'd'},
    {"debug", no_argument, 0, 'g'},
    {"iterations", required_argument, 0, 'i'},
    {"seed", required_argument, 0, 's'},
    {"help", no_argument, 0, 'h'},
    {"stencil", required_argument, 0, 't'},
    {"verbose", no_argument, 0, 'v'},
    {"workload", required_argument, 0, 'w'},
    {"innerloop", required_argument, 0, 'l'},
    {"rabbitmessage", required_argument, 0, 'm'},
    {"io-size", required_argument, 0, 'z'},
    {0, 0, 0, 0}
};

static char *shortargs = (char *)"a:b:d:i:n:s:t:l:hgvw:m:z:";

void usage(char *progname)
{
    printf("usage: %s [-w workload-type] [-d distribution] [-a workload-aval] [-b workload-bval] [-i iterations] [-s initial seed]\
            [-t stencil_size] [-l inner loop iterations] [-m rabbit message mode (0/1)] [-v] [-g] filename\n", progname);

    return;
}


void reduce_workload_max(struct coll_time *times_buffer, int iterations)
{
    double *workload = (double *)calloc(iterations, sizeof(double));
    double *workload_max = (double *)calloc(iterations, sizeof(double));

    /* First collect local data into the local buffer */
    for (int i = 0; i < iterations; i++) {
        workload[i] = times_buffer[i].bstart - times_buffer[i].start;
    }
    /* Now use an MPI Reduce to take the maximum of these expected times across all ranks */
    MPI_Allreduce(workload, workload_max, iterations, MPI_DOUBLE, MPI_MAX, my_comm);

    /* And put the actual maxes collected back into the times buffer */
    for (int i = 0; i < iterations; i++) {
        times_buffer[i].workload_max = workload_max[i];
    }
    free(workload);
    free(workload_max);

}

int main(int argc, char *argv[])
{
    int rank, nprocs, ret;
    gsl_rng * r;
    char exp[256];
    char *outfile = NULL;
    int optindex;
    char c;
    int stencil_size=0;
    int innerloop_itr = 1;
    MPI_Init(&argc,&argv);
    my_comm = MPI_COMM_WORLD;
    MPI_Comm_size(my_comm, &nprocs);
    MPI_Comm_rank(my_comm, &rank);
    amqp_connection_state_t conn = NULL;

    for (int i = 0; i < argc; i++)
        printf("%s ", argv[i]);
    printf("\n");

    /* Now that we've setup MPI, process remaining arguments for 
     * this application. */
    while ((c = getopt_long(argc, argv, shortargs, longargs, &optindex)) != -1)
    {
        switch (c) {
            case 'a':
                sscanf(optarg, "%lf", &a);
                break;
            case 'b':
                sscanf(optarg, "%lf", &b);
                break;
            case 'd':
                strncpy(distribution, optarg, 256);
                break;
            case 'i':
                sscanf(optarg, "%lu", &iterations);
                break;
            case 'v':
                verbose = 1;
                break;
            case 's':
                sscanf(optarg, "%lu", &initseed);
                break;
            case 'h':
                usage(argv[0]);
                exit(0);
                break;
            case 'm':
                makeRabbitCalls = 1;
                sscanf(optarg, "%s", rabbitIP);
                break;
            case 't':
                sscanf(optarg, "%d", &stencil_size);
                break;
            case 'l':
                sscanf(optarg, "%d", &innerloop_itr);
                break;
            case 'g':
                DEBUG = 1;
                break;
            case 'w':
                workload_str = optarg;
                if (strcmp(optarg, "sleep") == 0) workload = WORKLOAD_SLEEP;
                else if (strcmp(optarg, "lammps") == 0) workload = WORKLOAD_LAMMPS;
                else if (strcmp(optarg, "hpcg") == 0) workload = WORKLOAD_HPCG;
                else if (strcmp(optarg, "spmv") == 0) workload = WORKLOAD_SPMV;
                else if (strcmp(optarg, "fwq") == 0) workload = WORKLOAD_FWQ;
                else if (strcmp(optarg, "dgemm") == 0) workload = WORKLOAD_DGEMM;
                //			else if (strcmp(optarg, "stream") == 0) workload = WORKLOAD_STREAM;
                //			else if (strcmp(optarg, "fbench") == 0) workload = WORKLOAD_FBENCH;
                else if (strcmp(optarg, "io") == 0) workload = WORKLOAD_IO;
                else {
                    fprintf(stderr, "Unknown workload type %s.\n", optarg);
                    usage(argv[0]);
                    exit(0);
                }
                break;
            case 'z':
                sscanf( optarg, "%lu", &io_params.io_size );
                break;
            case '?':
            default:
                /* getopt_long already printed an error message. */
                usage(argv[0]);
                exit(-1);
                break;
        }
    } 

    /* We should have one argument left - the filename. */
    if (optind + 1 != argc) {
        printf("No filename given.\n");
        usage(argv[0]);
        exit(-1);
    }
    outfile = argv[optind];

    //Do we need to perform rabbit calls>?
    if(makeRabbitCalls){
        //reading configs from file and openning amqp conn
        conn = getAmqpConnection();
        // if this returns then we know the connection has been established
    }

    /*
     *  We use GSL (GSU Scientific Library) + SPRNG (The Scalable Parallel 
     *  Random Number Generators Library) for random numbers. 
     *  GSL provides the generation of random variables using different 
     *  random distributions while SPRNG adds to GSL the capacity to 
     *  generate independent streams of random variables across MPI ranks
     *
     * We seed the RNG based on the experiment we're running, including number
     * of ranks but not our particular rank. This gives the RNG a consistent seed so 
     * we can reproduce it.
     */
    snprintf(exp, 256, "%lu%32s%f%f%lu%d", 
            initseed, distribution, a, b, iterations, nprocs);
    unsigned long seed = hash_string(exp);
    gsl_rng_default_seed = seed;
    r = gsl_rng_alloc (gsl_rng_sprng50);

    double cpn = get_clocks_per_nanosecond();
    struct coll_time *times_buffer = (struct coll_time *)calloc(iterations, sizeof(struct coll_time));

    if (stencil_size < 0) {
        stencil_size = 0;
    }

    ret = barrier_loop(a, b, distribution, stencil_size, innerloop_itr, iterations, 
            times_buffer, cpn, r, conn);
    //closing the connection ro rabbit broker
    if(makeRabbitCalls){
        shutdownAmpq(conn);
        if(DEBUG){
            fprintf(stdout, "completed; closing the connection\n"); 
        }
    }
    set_experiment_id(r);

    reduce_workload_max(times_buffer, iterations);

    if (!ret && ((rank == 0) || verbose)){
        write_buffer(a, b, distribution, stencil_size, iterations, innerloop_itr, times_buffer, r,
                outfile);
    }
    free(times_buffer);
    free(r);

    MPI_Finalize();
}
