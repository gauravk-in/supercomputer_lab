/*
 * relax_jacobi.c
 *
 * Jacobi Relaxation
 *
 */

#include "heat.h"

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


/*
 * One Jacobi iteration step
 */
double relax_jacobi_return_residual( double *u, double *utmp,
		unsigned sizex, unsigned sizey )
{
	int i, j;
	double unew, diff, sum=0.0;

	for( i=1; i<sizey; i++ )
	{
		for( j=1; j<sizex-1; j++ )
		{
			if(i<sizey-1)
			{
				utmp[i*sizex + j]= 0.25 * (u[ i*sizex+j -1 ]+  // left
						u[ i*sizex+j +1 ]+  // right
						u[ (i-1)*sizex + j ]+  // top
						u[ (i+1)*sizex + j ]); // bottom
			}

			//Residual Calculation
			if(i>1)
			{
				unew = 0.25 * (utmp[ (i-1)*sizex+j -1 ]+  // left
						utmp[ (i-1)*sizex+j +1 ]+  // right
						utmp[ (i-2)*sizex + j ]+  // top
						utmp[ (i)*sizex + j ]); // bottom

				diff = unew - utmp[(i-1)*sizex+j];
				sum += diff * diff;
			}
		}
	}
	return sum;
}
