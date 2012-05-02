/*
 * relax_jacobi.c
 *
 * Jacobi Relaxation
 *
 */

#include "heat.h"

#if 0
/*
 * Residual (length of error vector)
 * between current solution and next after a Jacobi step
 */
double residual_jacobi( double *u, 
			unsigned sizex, unsigned sizey )
{
    unsigned i, j;
    double unew, diff, sum=0.0;

    for( i=1; i<sizey-1; i++ )
    {
	for( j=1; j<sizex-1; j++ )
        {
	    unew = 0.25 * (u[ i*sizex     + (j-1) ]+  // left
			   u[ i*sizex     + (j+1) ]+  // right
			   u[ (i-1)*sizex + j     ]+  // top
			   u[ (i+1)*sizex + j     ]); // bottom

	    diff = unew - u[i*sizex + j];
	    sum += diff * diff; 
	}
    }

    return sum;
}
#endif

/*
 * One Jacobi iteration step
 */
double relax_jacobi_return_residual( double *u, double *utmp,
		unsigned sizex, unsigned sizey, unsigned BlockSize )
{
	int i, j, k, l;
	double unew, diff, sum=0.0;

	unsigned BlockCountX = (sizex-2)/BlockSize;
	unsigned BlockCountY = (sizey-2)/BlockSize;

	for ( k = 0; k < BlockCountY; k++)
		for ( l = 0; l < BlockCountX; l++)
		{
			for( i=1 + BlockSize*k; i <= BlockSize*(k+1); i++ )
				for( j=1 + BlockSize*l; j <= BlockSize*(l+1); j++ )
				{
					utmp[i*sizex+j]= 0.25 * (u[ i*sizex     + (j-1) ]+  // left
							u[ i*sizex     + (j+1) ]+  // right
							u[ (i-1)*sizex + j     ]+  // top
							u[ (i+1)*sizex + j     ]); // bottom

					diff = utmp[i*sizex + j] - u[i*sizex +j];
					sum += diff * diff;
				}
		}
	return sum;
}
