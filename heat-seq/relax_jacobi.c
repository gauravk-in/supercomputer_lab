/*
 * relax_jacobi.c
 *
 * Jacobi Relaxation
 *
 */

#include "heat.h"

#if 0			//residual_jacobi
/*
 * Residual (length of error vector)
 * between current solution and next after a Jacobi step
 */
double residual_jacobi( double *u,
		unsigned sizex, unsigned sizey)
{
    unsigned i, j;
    double unew, diff, sum=0.0;

    for( i=1; i<sizey-1; i++ )
    {
	    for( j=1; j<sizex-1; j++ )
	    {
		    unsigned offset = i*sizex + j;
		    unew = 0.25 * (u[ offset -1 ]+  // left
				    u[ offset +1 ]+  // right
				    u[ offset - sizex ]+  // top
				    u[ offset + sizex ]); // bottom

		    diff = unew - u[offset];
		    sum += diff * diff;
	    }
    }

    return sum;

}
#endif			//residual_jacobi

/*
 * One Jacobi iteration step
 */
double relax_jacobi_return_residual( double *u, double *utmp,
		unsigned sizex, unsigned sizey, int Interleaving_Count )
{
	int i, j, k, l=0;
	double unew, diff, sum=0.0;
	unsigned long offset;

	//MODIFIED: exchanged the outer and the inner loop
	for( i=1; i < sizey-2+Interleaving_Count; i++ )
	{
		for(k=i; k > i-Interleaving_Count; k--)
		{
			for( j=1; j<sizex-1; j++ )
			{
				if(k == 0) break;
				if(k > sizey-2) continue;
			
				offset = k*sizex + j;

				//MODIFIED: storing the offset in a variable
				u[offset] = utmp[offset];
				utmp[offset]= 0.25 * (u[ offset -1 ]+  // left-old
						utmp[ offset +1 ]+  // right-new
						u[ offset - sizex ]+  // top-old
						utmp[ offset + sizex ]); // bottom-new
				if(k == i-Interleaving_Count+1)
				{
					diff = utmp[offset] - u[offset];
					sum += diff * diff;
				}
			}
		}
	}
	return sum;
}
