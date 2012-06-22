/*
 * heat.h
 *
 * Global definitions for the iterative solver
 */

#ifndef JACOBI_H_INCLUDED
#define JACOBI_H_INCLUDED

#include <stdio.h>
#include <mpi.h>

// configuration

typedef struct
{
    float posx;
    float posy;
    float range;
    float temp;
}
heatsrc_t;

typedef struct
{
    unsigned maxiter;       // maximum number of iterations
    unsigned act_res;
    unsigned max_res;       // spatial resolution
    unsigned initial_res;
    unsigned res_step_size;
    unsigned blocksize_x;
    unsigned blocksize_y;
    int algorithm;          // 0=>Jacobi, 1=>Gauss

  
    double *u, *uhelp;
    double *sendbuf_left, *recbuf_left, *sendbuf_right, *recbuf_right;
    double *sendbuf_top, *recbuf_top, *sendbuf_bottom, *recbuf_bottom;

    unsigned   numsrcs;     // number of heat sources
    heatsrc_t *heatsrcs;
    int thread_dims[2]; 	//x*y dimensions of thread

}
algoparam_t;


// function declarations

// misc.c
int initialize( algoparam_t *param, int *coords );
int finalize( algoparam_t *param );

// Gauss-Seidel: relax_gauss.c
double relax_gauss_return_residual( algoparam_t *param, int interleaving_count, int *coords, MPI_Comm comm2d);
// Jacobi: relax_jacobi.c

double relax_jacobi_return_residual( double *u, double *utmp,
		   unsigned sizex, unsigned sizey ); 


#endif // JACOBI_H_INCLUDED
