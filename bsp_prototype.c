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
#include <openssl/ssl.h>
#include <math.h>
#include <getopt.h>

#include <gsl/gsl_rng.h>
#include "gsl-sprng.h"
#include <gsl/gsl_randist.h>

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

unsigned char DEBUG = 0;
struct coll_time{
	int rank;
  	double expected_sleep;
	double wait;
  	double bstart;
  	double bend;
  	double sleep;
};

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
	else return RNG_ERROR;
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
 * Main loop for the program - sleep in a barrier some number of iterations
 * with the length of each iteration drawn from a random distribution. 
 * Log the desired local, actual local, and actual global sleep times
 * as we run for later output.
 * Also if stencil_size > 0, each node halos stencil_size doubles with its neighbors
 * in asnearly a square topology as we can find.
 */

int barrier_loop(double a, double b, char * distribution, int stencil_size, int iterations, 
		 struct coll_time * times_buffer, double cpn, gsl_rng *r)
{
  	double coll_exp_sleep = 0.0;
  	double waitall_start = 0.0;
  	double coll_start = 0.0;
  	double coll_end = 0.0;
  	double coll_sleep = 0.0;
  	double rank_start_time = 0.0;
  	int i;
  	double inter_time = 0;
  	int rank = 0;
  	int nprocs = 0;
    	MPI_Request requests[8];
	MPI_Comm my_comm = MPI_COMM_WORLD;
 	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	for (i = 0; i < 8; i++)
		requests[i] = MPI_REQUEST_NULL;

  	enum rng_type rng_type = init_rng_type(distribution);
  	if (rng_type < 0) {
      		return -1;
  	}
	//variables for decomposing the grid!
	//buffers for doing p2p ops
	int myLeftNbr, myRightNbr, myTopNbr, myBottomNbr;
	double* topBuffer, *downBuffer, *rightBuffer, *leftBuffer, *myValues;
	if (stencil_size){
		int grid[2];
		int period[2] = {1, 1};
		grid[0] = 0; grid[1] = 0;
		MPI_Dims_create(nprocs, 2, grid);
		if (rank == 0 && DEBUG){
			printf("There are %d cores in x and %d cores in y\n", grid[0], grid[1]);
		}
		period[0] = 1; period[1] = 1;
		MPI_Cart_create(MPI_COMM_WORLD, 2, grid, period, 1, &my_comm);
 		MPI_Comm_rank(my_comm, &rank);
		MPI_Cart_shift(my_comm, 0, -1, &rank, &myLeftNbr);
		MPI_Cart_shift(my_comm, 1, -1, &rank, &myBottomNbr);
		MPI_Cart_shift(my_comm, 0, 1, &rank, &myRightNbr);
		MPI_Cart_shift(my_comm, 1, 1, &rank, &myTopNbr);
		
		myValues = (double*) myAlloc(4 * stencil_size * sizeof(double));
		for(int i=0; i < 4*stencil_size; i++){
			myValues[i] = double(rank);
		}
		topBuffer = (double*) myAlloc(stencil_size * 2 * sizeof(double));
		downBuffer = topBuffer + stencil_size;
    		leftBuffer = (double*) myAlloc(stencil_size * 2 * sizeof(double));
		rightBuffer = leftBuffer + stencil_size;
	}
	rank_start_time = MPI_Wtime();

	/* We start at -5 to do 5 warmup iterations that are recorded */
  	for( i = -5; i < iterations; i++) {
		if (stencil_size){
			//top bottom right left
			MPI_Irecv(topBuffer, stencil_size, MPI_DOUBLE, myTopNbr, 811, MPI_COMM_WORLD, &requests[0]);
			MPI_Irecv(downBuffer, stencil_size, MPI_DOUBLE, myBottomNbr, 823, MPI_COMM_WORLD, &requests[1]);
			MPI_Irecv(rightBuffer, stencil_size, MPI_DOUBLE, myRightNbr, 919, MPI_COMM_WORLD, &requests[2]);
			MPI_Irecv(leftBuffer, stencil_size, MPI_DOUBLE, myLeftNbr, 977, MPI_COMM_WORLD, &requests[3]);
		}
		//now sleep which counts as our computation
		inter_time = generate_interval_rng(r, rng_type, a, b);
		assert(inter_time >= 0.0 );

		coll_sleep = MPI_Wtime();
		coll_exp_sleep = inter_time;
		if (inter_time > 0){
			sleep_rdtsc(1000  * inter_time, cpn);
		}
		waitall_start = MPI_Wtime();
		//now sends
		if(stencil_size){
			MPI_Isend(myValues, stencil_size, MPI_DOUBLE, myBottomNbr, 811, MPI_COMM_WORLD, &requests[4]);
			MPI_Isend(myValues + stencil_size, stencil_size, MPI_DOUBLE, myTopNbr, 823, MPI_COMM_WORLD, &requests[5]);
			MPI_Isend(myValues + 2*stencil_size, stencil_size, MPI_DOUBLE, myLeftNbr, 919, MPI_COMM_WORLD, &requests[6]);
			MPI_Isend(myValues + 3*stencil_size, stencil_size, MPI_DOUBLE, myRightNbr, 977, MPI_COMM_WORLD, &requests[7]);
			MPI_Waitall(8, requests, MPI_STATUSES_IGNORE);
		}
		coll_start = MPI_Wtime();
    		MPI_Barrier(MPI_COMM_WORLD);
    		coll_end = MPI_Wtime();
		
    		waitall_start =  ( waitall_start - rank_start_time ) * SECOND_TO_MICRO_FACTOR;
    		coll_start =     ( coll_start - rank_start_time ) * SECOND_TO_MICRO_FACTOR;
    		coll_end   =     ( coll_end   - rank_start_time ) * SECOND_TO_MICRO_FACTOR;
    		coll_sleep =     ( coll_sleep - rank_start_time ) * SECOND_TO_MICRO_FACTOR;

		/* Don't record warmup iterations  */
		if (i >= 0) {
			times_buffer[ i ].rank = rank;
			times_buffer[ i ].wait = waitall_start;
			times_buffer[ i ].bstart = coll_start;
			times_buffer[ i ].bend =  coll_end;
			times_buffer[ i ].expected_sleep = coll_exp_sleep;
			times_buffer[ i ].sleep = coll_sleep;
		}
	}// end of main loop
	//freeing the bufferes used for MPI exchange operations
	if(stencil_size){
		free(topBuffer);
		free(leftBuffer);
		free(myValues);
	}
	return 0;
}


void write_buffer(double a, double b, char * distribution, int stencil_size, int iterations, 
		  struct coll_time * times_buffer, gsl_rng *r, char *outfile)
{
	const char *str = "0123456789abcdef";
	char experimentID[20];
  	FILE *f_time;
	int rank;
  	int root = 0;
  	int i;
	int length = 13;

  	f_time = fopen(outfile, "w");

	/* Share the experiment ID across ranks */
  	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  	if (rank == 0) {
		// We use GSL here, so we'll get a fixed experiment ID for a 
		// fixed set of parameters, which is a good thing because it
		// lets us consistently regenerate experiments.
    		for (i = 0; i < length; i++) {
      			experimentID[i] = str[gsl_rng_uniform_int(r, 16)];
    		}
    		experimentID[i] = 0;
  	}
  	MPI_Bcast(&experimentID, sizeof(experimentID),
                  MPI_CHAR, root, MPI_COMM_WORLD);

	/* Print the logged data to the local data file */
	fprintf(f_time, "[\n");
  	for (i = 0; i < iterations; i++) {
	    	fprintf( f_time, "{ " );
		fprintf( f_time, " \"uniq_id\": \"%s\", "
		     	" \"rank\": %d, "
		     	" \"iteration\": %d, "
		     	" \"distribution\": \"%s\", "
		     	" \"a\": %f, "
		     	" \"b\": %f, "
			" \"stencil_size\": %d, "
		     	" \"iterations\": %d, ",
		     	experimentID, rank, i,
		     	distribution, a, b, stencil_size, iterations
			);
		fprintf( f_time, 
		    	" \"sleep_start\": %.3lf, "
		     	" \"wait_start\": %.3lf, "
		     	" \"barrier_start\": %.3lf, "
		     	" \"barrier_end\": %.3lf, "
		     	" \"expected_sleep_usec\": %.3lf, "
		     	" \"actual_sleep_usec\": %.3lf "
		     	" }",
		     	times_buffer[i].sleep         ,
		     	times_buffer[i].wait          ,
		     	times_buffer[i].bstart         ,
		     	times_buffer[i].bend           ,
		     	times_buffer[i].expected_sleep,
		     	times_buffer[i].bstart - times_buffer[i].sleep );
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
static unsigned int verbose = 1;

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
	{"reduced", no_argument, 0, 'r'},
	{0, 0, 0, 0}
};

static char *shortargs = (char *)"a:b:d:i:s:n:t:hgr";

void usage(char *progname)
{
	printf("usage: %s [-d dist] [-a aval] [-b bval] [-i iterations] [-s initial seed] [-t stencil_size] [-r] [-g] filename\n", progname);

	return;
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

	MPI_Init(&argc,&argv);
  	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

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
		case 'r':
			verbose = 0;
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
      		case 'g':
        		DEBUG = 1;
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

	if (stencil_size < 0) stencil_size = 0;
  	ret = barrier_loop(a, b, distribution, stencil_size, iterations, 
			   times_buffer, cpn, r);

  	if (!ret && ((rank == 0) || verbose)){
		write_buffer(a, b, distribution, stencil_size, iterations, times_buffer, r,
			     outfile);
	}
	free(times_buffer);
  	free(r);

  	MPI_Finalize();
}
