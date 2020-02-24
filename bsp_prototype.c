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

//Stencil Radius
#define RADIUS 1
#define SECOND_TO_MICRO_FACTOR 1000000
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
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
unsigned char DEBUG = 0;
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
	WORKLOAD_IOR
};
enum bsp_workload workload = WORKLOAD_SLEEP;
char *workload_str = "sleep";

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

void fill(double *p, int n)
{
	for (int i = 0; i < n; ++i)
		p[i] = 2 * drand48() - 1;
}

int init_workload(int w, gsl_rng *r, char *distribution, double a, double b)
{
	double *buf;
  rng_type = init_rng_type(distribution);
	switch (w) {
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
	default:
		assert(0 && "Unknown workload!");
	}
	return 0;
}


/* Do one iteration of whatever comoute workload was requested */
void run_workload(int w, gsl_rng *r, double a, double b, double cpn)
{
  	double inter_time = 0;
	switch(w) {
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
		 struct coll_time * times_buffer, double cpn, gsl_rng *r)
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

	for (i = 0; i < 8; i++)
		requests[i] = MPI_REQUEST_NULL;

	if (init_workload(workload, r, distribution, a, b)) {
		return -1;
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
	}// end of main loop
	//freeing the bufferes used for MPI exchange operations
	if(stencil_size){
		tear_down_stencil(right_buf, left_buf, up_buf, down_buf, values);
	}
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
    char hostbuffer[256];
    int hostname_ret;

    // To retrieve hostname 
    hostname_ret = gethostname(hostbuffer, sizeof(hostbuffer));
    if (hostname_ret) {
      fprintf(stderr,"Error: Hostname not found.\n");
      exit(-1);
    }

    // 256+256+"_"+"_" = 514
    char hostname_outfile[514];
    int hostname_idx = -1;

    // Get the last dir '/'
    int last_dir_idx = 0;
    for (int i=0; i<256; i++) {
      if (!outfile[i]) {
        break;
      }
      if (outfile[i] == '/') {
        last_dir_idx=i;
      }
    }

    // Write the path up to last '/'
    for (int i=0; i<256; i++) {
      if (i <= last_dir_idx) {
        hostname_outfile[i] = outfile[i];
      }
    }

    // Write the hostname
    for (int i=last_dir_idx+1; i<256; i++) {
      if (hostbuffer[i-last_dir_idx-1]) {
        hostname_outfile[i] = hostbuffer[i-last_dir_idx-1];
      }
      else {
        hostname_outfile[i]='_';
        hostname_idx=i+1;
        break;
      }
    } 

    // Write the rest of the filename after hostname
    for (int i = 0; i<256; i++) {
      if (outfile[i+last_dir_idx+1]) {
        hostname_outfile[i+hostname_idx] = outfile[i+last_dir_idx+1];
      } else { hostname_outfile[i+hostname_idx]='\0'; break; }
    }


  	MPI_Comm_rank(my_comm,&rank);
  	MPI_Comm_size(my_comm,&nproc);

	  /* Print the logged data to the local data file */
  	f_time = fopen(hostname_outfile, "w");
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
	{0, 0, 0, 0}
};

static char *shortargs = (char *)"a:b:d:i:n:s:t:l:hgvw:";

void usage(char *progname)
{
	printf("usage: %s [-w workload-type] [-d distribution] [-a workload-aval] [-b workload-bval] [-i iterations] [-s initial seed]\
	 [-t stencil_size] [-l inner loop iterations] [-v] [-g] filename\n", progname);

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
				else if (strcmp(optarg, "dgemm") == 0) workload = WORKLOAD_DGEMM;
	//			else if (strcmp(optarg, "stream") == 0) workload = WORKLOAD_STREAM;
	//			else if (strcmp(optarg, "fbench") == 0) workload = WORKLOAD_FBENCH;
	//			else if (strcmp(optarg, "ior") == 0) workload = WORKLOAD_IOR;
				else {
					fprintf(stderr, "Unknown workload type %s.\n", optarg);
					usage(argv[0]);
					exit(0);
				}
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
			times_buffer, cpn, r);

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
