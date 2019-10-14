/*
 * Copyright 2019. Keira Haskins and Patrick G. Bridges, University of New Mexico
 * Copyright 2015. Oscar Mondragon
 * bsp_prototype.c (previously appGen.c)
 * Description:
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

#include <gsl/gsl_rng.h>
#include "gsl-sprng.h"
#include <gsl/gsl_randist.h>

//#include "sprng_cpp.h"

#include <assert.h>

#include "mpi.h"

#include <time.h>
#include <sys/time.h>

/*
 * If distribution is:
 * exponential,    a = interarrival mean
 * gaussian,     a = interarrival mean, b = interarrival stddev
 * flat,     a = start number, b = end number
 * pareto,    a = exponent (i.e., shape)
 *      b = scale (shift to the right ~ largest value in tail),
 * constant    a = interarrival constant duration, b does not have effect
 */

struct coll_time{
  int rank;
  unsigned long expected_sleep;
  unsigned long start;
  unsigned long end;
  unsigned long sleep;
};


/* 
 * Code to sleep for a pre-determined about of time on an x86_64 system as
 * closely as possible by busy-waiting on the time stamp counter 
 */
unsigned long long rdtsc(void)
{
  unsigned int lo, hi;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

/*
 * get_clocks_per_nanosecond and sleep_rdtsc are based on code in
 * http://sci.tuomastonteri.fi/programming/cplus/x86timer
 */

double
get_clocks_per_nanosecond() {

  int sum=0;
  int times=0;
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
  do
  {
    now=rdtsc();
  }while ( (now-begin) < dtime);
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
      			inter_time = gsl_ran_pareto (r,a,b);
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
/* 
 * Main loop for the program - sleep in a barrier some number of iterations
 * with the length of each iteration drawn from a random distribution. 
 * Log the desired local, actual local, and actual global sleep times
 * as we run for later output.
 */
int barrier_loop(double a, double b, char * distribution, int iterations, 
		 struct coll_time * times_buffer, double cpn, gsl_rng *r)
{
  	double coll_exp_sleep = 0.0;
  	double coll_start = 0.0;
  	double coll_end = 0.0;
  	double coll_sleep = 0.0;

  	double rank_start_time = 0.0;
  	double rank_end_time = 0.0;

  	int i;
  	double inter_time = 0;
  	int rank = 0;
  	int nprocs = 0;

 	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

  	enum rng_type rng_type = init_rng_type(distribution);
  	if (rng_type < 0) {
      		return -1;
  	}
  	double resolution = MPI_Wtick();

  	rank_start_time = MPI_Wtime();

  	for( i = 0; i < iterations; i++){
    		inter_time = generate_interval_rng(r, rng_type, a, b);
    
    		assert(inter_time >= 0.0 );

    		coll_sleep = MPI_Wtime();
    		coll_exp_sleep = inter_time;
    		if (inter_time > 0){
        		sleep_rdtsc(1000  * inter_time, cpn);
    		}

    		coll_start = MPI_Wtime();
    		MPI_Barrier(MPI_COMM_WORLD);
    		coll_end = MPI_Wtime();

    		coll_start =     ( coll_start - rank_start_time ) * 1000000000;
    		coll_end   =     ( coll_end   - rank_start_time ) * 1000000000;
    		coll_sleep =     ( coll_sleep - rank_start_time ) * 1000000000;
    		coll_exp_sleep = ( coll_exp_sleep ) * 1000;

    		times_buffer[ i ].rank = rank;
    		times_buffer[ i ].start = (unsigned long) coll_start;
    		times_buffer[ i ].end =  (unsigned long) coll_end;
    		times_buffer[ i ].expected_sleep = (unsigned long)coll_exp_sleep;
    		times_buffer[ i ].sleep = (unsigned long) coll_sleep;
  	}
  	return 0;
}

void write_buffer(double a, double b, char * distribution, int iterations, 
		  struct coll_time * times_buffer, gsl_rng *r, char *outfile)
{
	const char *str = "0123456789abcdef";
	char tmpbuff[64];
	//char hostname[1024];
	char out[20];
	char experimentID[20];
  	FILE *f_time;
	int rank;
  	int root = 0;
  	int i;
	int length = 13;
//	char tmpfile[512];
//	struct stat st = {0};

//	hostname[1023] = '\0';
//	gethostname(hostname, 1023);
	// Create new directories in tmp: /tmp/results/<DISTRIBUTION> to
	// store results before writing back to the file system.
//	strcpy(tmpfile, "/tmp/results");
//	if (stat(tmpfile, &st) == -1) {
//		mkdir(tmpfile, 0700);
//	}
//	strcat(tmpfile, "/");
//	strcat(tmpfile, hostname);
//	strcat(tmpfile, "_");

//	for (i = 0; i < length; i++) {
//		out[i] = str[gsl_rng_uniform_int(r, 16)];
//	}
 // 	out[i] = 0;
//  	strcat(tmpfile, out);
 // 	strcat(tmpfile, ".csv");

  	f_time = fopen(outfile, "a");

	/* Share the experiment ID across ranks */
  	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  	if (rank == 0) {
    		for (i = 0; i < length; i++) {
      			experimentID[i] = str[gsl_rng_uniform_int(r, 16)];
    		}
    		experimentID[i] = 0;
  	}
  	MPI_Bcast(&experimentID, sizeof(experimentID),
                  MPI_CHAR, root, MPI_COMM_WORLD);

	/* Print the logged data to the local data file */
  	for (i = 0; i < iterations; i++) {
		fprintf(f_time, "%s,", experimentID);
    		fprintf(f_time, "%d,", rank);
   		fprintf(f_time, "%lu,", 0);
    		fprintf(f_time, "%s,", distribution);
    		fprintf(f_time, "%f,", a);
    		fprintf(f_time, "%f,", b);
    		fprintf(f_time, "%u,", iterations);
    		fprintf(f_time, "%lu,%lu,%lu,%lu,%lu",
        			times_buffer[i].sleep         ,
        			times_buffer[i].start         ,
        			times_buffer[i].end           ,
        			times_buffer[i].expected_sleep,
        			times_buffer[i].start - times_buffer[i].sleep);
    		fprintf(f_time, "\n");
  	}
  	fprintf(f_time, "\n");
  	fclose(f_time);
}


int main(int argc, char *argv[])
{

	char distribution[20] = "";
	double a, b;
	int iterations = 0;
	int rank, nprocs, ret;
  	gsl_rng * r;

	if(argc != 6){
    		printf("Usage: %s outfile a b distribution iterations\n", 
		       argv[0]);
    		return -1;
  	}

	MPI_Init(&argc,&argv);
  	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

  	sscanf(argv[2], "%lf", &a);
  	sscanf(argv[3], "%lf", &b);
  	sscanf(argv[4], "%s", distribution);
  	iterations = atoi(argv[5]);


  	/*
   	 *  We use GSL (GSU Scientific Library) + SPRNG (The Scalable Parallel 
   	 *  Random Number Generators Library) for random numbers. 
   	 *  GSL provides the generation of random variables using different 
   	 *  random distributions while SPRNG adds to GSL the capacity to 
   	 *  generate independent streams of random variables across MPI ranks
   	 */
  	r = gsl_rng_alloc (gsl_rng_sprng20);

  	double cpn = get_clocks_per_nanosecond();
  	struct coll_time *times_buffer = (struct coll_time *)calloc(iterations, sizeof(struct coll_time));

  	ret = barrier_loop(a, b, distribution, iterations, times_buffer, 
			   cpn, r);
  	if (!ret)
		write_buffer(a, b, distribution, iterations, times_buffer, r,
			     argv[1]);

	free(times_buffer);
  	free(r);

  	MPI_Finalize();
}
