/*
 * heat.h
 *
 * Iterative solver for heat distribution
 */


#include <stdio.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1
#define INTERLEAVING_COUNT 10

#include "input.h"
#include "heat.h"
#include "timing.h"


void usage( char *s )
{
	fprintf(stderr, 
			"Usage: %s <input file> [result file]\n\n", s);
}


int main( int argc, char *argv[] )
{
	unsigned iter;
	FILE *infile, *resfile;
	char *resfilename;
	int periods[2]={FALSE, FALSE};
	MPI_Comm comm2d;

	int nthreads, rank;
	int thdx, thdy;

	double *tmp;

	// algorithmic parameters
	algoparam_t param;
	int np;

	double runtime, flop;
	double residual;
	
	int coords[2];

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nthreads);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	// check arguments
	if( argc < 2 )
	{
		usage( argv[0] );
		MPI_Finalize();
		return 1;
	}

	// check input file
	if( !(infile=fopen(argv[1], "r"))  ) 
	{
		fprintf(stderr, 
				"\nError: Cannot open \"%s\" for reading.\n\n", argv[1]);

		usage(argv[0]);
		MPI_Finalize();
		return 1;
	}

    // check input
 
    if( !read_input(infile, &param) )
    {
	fprintf(stderr, "\nError: Error parsing input file.\n\n");
      
	usage(argv[0]);
	MPI_Finalize();
	return 1;
    }


    MPI_Cart_create(MPI_COMM_WORLD, 2, param.thread_dims, periods, FALSE, &comm2d);
    MPI_Cart_coords(comm2d, rank, 2, coords);

    if(rank==0) 
	print_params(&param);

    param.u     = 0;
    param.uhelp = 0;
    param.sendbuf_left =0;
    param.sendbuf_right = 0;
    param.recbuf_left = 0;
    param.recbuf_right =0;
    param.sendbuf_top =0;
    param.sendbuf_bottom = 0;
    param.recbuf_top = 0;
    param.recbuf_bottom =0;

    param.act_res = param.initial_res;
 
    // loop over different resolutions
    while(1) {

	// free allocated memory of previous experiment
	if (param.u != 0)
	    finalize(&param);

	if( !initialize(&param, coords) )
	{
	    fprintf(stderr, "Error in Jacobi initialization.\n\n");

	    usage(argv[0]);
	}

	fprintf(stderr, "Resolution: %5u\r", param.act_res);

	// full size (param.act_res are only the inner points)
	np = param.act_res + 2;
    
	// starting time
	runtime = MPI_Wtime();
	residual = 999999999;
 	
	iter = 0;
	while(1) {

	    switch( param.algorithm ) {

		case 0: // JACOBI
      
		    residual = relax_jacobi_return_residual(param.u, param.uhelp, np, np);
			tmp = param.u;
			param.u = param.uhelp;
			param.uhelp = tmp;
		    break;

		case 1: // GAUSS
		    residual = relax_gauss_return_residual(&param, INTERLEAVING_COUNT, coords , comm2d);
		    break;
	    }
	    
	    iter+= INTERLEAVING_COUNT;

	    // solution good enough ?
	    if (residual < 0.000005) break;

	    // max. iteration reached ? (no limit with maxiter=0)
	    if (param.maxiter>0 && iter>=param.maxiter) break;
	    
	    if (iter % 100 == 0)
		fprintf(stderr, "residual %f, %d iterations\n", residual, iter);
	}
	
	// Flop count after <i> iterations
	flop = iter * 7.0 * param.act_res * param.act_res;
	// stopping time
	runtime = MPI_Wtime() - runtime;

	if (rank == 0)
	{
		fprintf(stderr, "Resolution: %5u, ", param.act_res);
		fprintf(stderr, "Time: %04.3f ", runtime);
		fprintf(stderr, "(%3.3f GFlop => %6.2f MFlop/s, ", 
			flop/1000000000.0,
			flop/runtime/1000000);
		fprintf(stderr, "residual %f, %d iterations)\n", residual, iter);

		// for plot...
		printf("%5d %f\n", param.act_res, flop/runtime/1000000);
	}

	if (param.act_res + param.res_step_size > param.max_res) break;
	param.act_res += param.res_step_size;
    }

    finalize( &param );
    MPI_Finalize();
    return 0;
}
