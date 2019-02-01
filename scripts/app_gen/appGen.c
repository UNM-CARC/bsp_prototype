/*
 * Copyright(C) 2015. Oscar Mondragon
 * appGen.c
 * Description: 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

//#include "gsl-sprng.h"
#include "sprng_cpp.h"

#include <assert.h>

#include "mpi.h" 

#include <time.h>



/*
 * If distribution is: 
 * exponential,		a = interarrival mean
 * gaussian, 		a = interarrival mean, b = interarrival sttdev
 * flat, 		a = start number, b = end number
 * pareto,		a = exponent (i.e., shape)
 *			b = scale (shift to the right ~ largest value in tail), 
 * constant		a = interarrival constant duration, b does not have effect
 */

struct coll_time{
	int rank;
	unsigned long start;
	unsigned long end;
};


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

uint64_t
get_clocks_per_nanosecond() {

	int sum=0;
	int times=0;
	uint64_t bench1=0;
	uint64_t bench2=0;
	uint64_t clocks_per_nanosecond=0;

	bench1=rdtsc();

	usleep(250000); // 1/4 second

	bench2=rdtsc();

	clocks_per_nanosecond =	 bench2-bench1;
	clocks_per_nanosecond *= 4.0e-9;

	return clocks_per_nanosecond;
}

void sleep_rdtsc(uint64_t nanoseconds, uint64_t clocks_per_nanosecond)
{
	uint64_t begin=rdtsc();
	uint64_t now;
	uint64_t dtime = nanoseconds*clocks_per_nanosecond;
	do
	{
		now=rdtsc();
	}while ( (now-begin) < dtime);
}



int 
barrier_loop(double a, double b, char * distribution, int iterations, struct coll_time * times_buffer, uint64_t cpn){


#ifdef USE_METRICS	
	
	FILE *f;
	f = fopen("result_coll", "a");

	double coll_start = 0.0;
	double coll_end = 0.0;
	
	double rank_start_time = 0.0;	
	double rank_end_time = 0.0;	
	
#endif
	double app_start_time = 0.0;
	double app_end_time = 0.0;

	FILE *f_time;
        char tmpfile[512];
        char tmpbuff[12];
        struct stat st = {0};
        // Create new directories in tmp: /tmp/results/<DISTRIBUTION> to 
        // store results before writing back to the file system.
        strcpy(tmpfile, "/tmp/results");
        if (stat(tmpfile, &st) == -1) {
          mkdir(tmpfile, 0700);
        }
        strcat(tmpfile, "/");
        strcat(tmpfile, distribution);
        st = {0};

        if (stat(tmpfile, &st) == -1) {
          mkdir(tmpfile, 0700);
        }
        strcat(tmpfile, "/");
        strcat(tmpfile, "mean_");
        sprintf(tmpbuff, "%.0f", a);
        strcat(tmpfile, tmpbuff);
        strcat(tmpfile, "_");
	strcat(tmpfile, "stddev_");
        sprintf(tmpbuff, "%.0f", b);
        strcat(tmpfile, tmpbuff);
        strcat(tmpfile, "_");
	strcat(tmpfile, "iterations_");
        sprintf(tmpbuff, "%.0f", iterations);
        strcat(tmpfile, tmpbuff);
        strcat(tmpfile, "time_ns");
        f_time = fopen(tmpfile, "a");
        //printf("PATH = %s\n", tmpfile);

        //printf("Failed to open tmp file\n");
        //f_time = fopen("result_time_ns", "a");
        
	int i =0;
	double inter_time = 0;	
	int rank = 0;
	int nprocs = 0;	
	gsl_rng * r;

	MPI_Comm_rank(MPI_COMM_WORLD,&rank); 
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 

	/*
         *  We use GSL (GSU Scientific Library) + SPRNG (The Scalable Parallel Random Number Generators Library)
         *  GSL provides the generation of random variables using different random distributions
	 *  SPRNG adds to GSL the capacity to generate independent streams of random variables across MPI ranks
	 */
	
	r = gsl_rng_alloc (gsl_rng_ran2);

	double resolution = MPI_Wtick();
	
#ifdef USE_METRICS	
	rank_start_time = MPI_Wtime();
#endif	

	if (rank == 0)
		app_start_time = MPI_Wtime();


	//printf("Time resolution %f\n", resolution);
	//printf("Setting start time %f\n", rank_start_time);

	for( i = 0; i < iterations; i++){
	
		if (strcmp(distribution,"gaussian") == 0){

			double inter_mean = a;
			double inter_sttdev = b;		

			inter_time =  gsl_ran_gaussian (r, inter_sttdev) + inter_mean;
			// Avoid inter arrival time zero
			while(inter_time == 0)
				inter_time =  gsl_ran_gaussian (r, inter_sttdev) + inter_mean;
		}
	
		else if (strcmp(distribution,"exponential") == 0){
			
			double inter_mean = a;
			
			inter_time =  gsl_ran_exponential (r, inter_mean);
			// Avoid inter arrival time zero
			while(inter_time == 0)
				inter_time =  gsl_ran_exponential (r, inter_mean);
		}	

	
		else if (strcmp(distribution,"flat") == 0){
	
			double start = a;
			double end = b;		

			inter_time = gsl_ran_flat (r,a,b);

			// Avoid inter arrival time zero
			while(inter_time == 0)
				inter_time = gsl_ran_flat (r,a,b);
		
		}
	
		else if (strcmp(distribution,"pareto") == 0){
	
			double exponent = a;
			double scale = b;		

			inter_time = gsl_ran_pareto (r,a,b);

			// Avoid inter arrival time zero
			while(inter_time == 0)
				inter_time = gsl_ran_pareto (r,a,b);
		
		}
	
		else if (strcmp(distribution,"constant") == 0){
	
			inter_time = a;

		}


		else{
			printf("Please provide a valid distribution\n");
			return -1;
		}

		//printf("(rank %d, it %d) inter_time %f\n", rank, i, inter_time);
		
		assert(inter_time >= 0 );
	
		/* If computation time < 1ms use 
		 * rdtsc + busy-waiting delay approach
		 * otherwise use usleep()
		 */
		
		if (inter_time > 0){

			if (inter_time < 1000)
				sleep_rdtsc(1000  * inter_time, cpn);
			else
				usleep(inter_time);		
		}

#ifdef USE_METRICS	
		coll_start = MPI_Wtime();
#endif		
		MPI_Barrier(MPI_COMM_WORLD);
		
#ifdef USE_METRICS	
		coll_end = MPI_Wtime();
#endif

		//printf("rank %d MPI_Barrier %lu %lu\n", rank, (unsigned long)coll_start, (unsigned long)coll_end );
		
#ifdef USE_METRICS	
		coll_start = ( coll_start - rank_start_time ) * 1000000000;
		coll_end   = ( coll_end - rank_start_time ) * 1000000000;

		times_buffer[ i ].rank = rank; 
		times_buffer[ i ].start = (unsigned long) coll_start; 
		times_buffer[ i ].end =  (unsigned long) coll_end; 
#endif
	}
	
#ifdef USE_METRICS	
	for (i = 0; i < iterations;  i++ ){

		//printf("iterations %d\n",i);

		fprintf(f, "rank %d MPI_Barrier %lu %lu\n", 
			times_buffer[i].rank, 
			times_buffer[i].start, 
			times_buffer[i].end);
	}
#endif
	
	if(rank ==0){
		// Time ns	
		app_end_time   = 1000000000 * MPI_Wtime();
		app_start_time = 1000000000 * app_start_time;	

		fprintf(f_time, "%lu\n", 
	        	(unsigned long) (app_end_time - app_start_time) );
	        printf("%lu\n", (unsigned long)(app_end_time - app_start_time));
	
	}

	free(r);
	fclose(f_time);

#ifdef USE_METRICS	
	fclose(f);
#endif	

	return 0;
}


int 
main(int argc, char *argv[]){
	
	char distribution[20] = "";
	double inter_mean = 0;
	double inter_sttdev = 0;
	int iterations = 0;
	int rank, nprocs;
	
	if(argc < 5){
		printf("Usage: app_gen <a> <b> <computation distribution> <iterations> \n");
		return -1;
	}

	MPI_Init(&argc,&argv); 
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs); 
	MPI_Comm_rank(MPI_COMM_WORLD,&rank); 

	sscanf(argv[1], "%lf", &inter_mean);
	sscanf(argv[2], "%lf", &inter_sttdev);
	sscanf(argv[3], "%s", distribution);
	iterations = atoi(argv[4]);

	struct coll_time times_buffer[ iterations ];
	uint64_t cpn = get_clocks_per_nanosecond();
	
	barrier_loop(inter_mean, inter_sttdev, distribution, iterations, times_buffer,cpn);

	MPI_Finalize();

	return 0;

}