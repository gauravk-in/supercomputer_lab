/*
 * relax_jacobi.c
 *
 * Jacobi Relaxation
 *
 */

#include "heat.h"


/*
 * One Jacobi iteration step
 */
double relax_jacobi_return_residual( double *u, double *utmp,
		unsigned sizex, unsigned sizey )
{
	int i, j;
	double diff, sum=0.0;

	#pragma omp parallel shared(u,utmp,sizex,sizey) private(i,j,diff) reduction(+:sum) num_threads(16)
	{
		#pragma omp for schedule(static) 
		for( i=1; i<sizey-1; i++ )
		{
			for( j=1; j<sizex-1; j++ )
			{

				utmp[i*sizex + j]= 0.25 * (u[ i*sizex+j -1 ]+  // left
						u[ i*sizex+j +1 ]+  // right
						u[ (i-1)*sizex + j ]+  // top
						u[ (i+1)*sizex + j ]); // bottom

				diff = utmp[i*sizex + j] - u[i*sizex +j];
				sum += diff*diff;

			}
		}
	}
	return sum;
}
