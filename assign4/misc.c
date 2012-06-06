/*
 * misc.c
 *
 * Helper functions for
 * - initialization
 * - finalization,
 * - writing out a picture
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "heat.h"

/*
 * Initialize the iterative solver
 * - allocate memory for matrices
 * - set boundary conditions according to configuration
 */
int initialize( algoparam_t *param )
{
    int i, j;
    double dist;

    // total number of points 
    const int np = param->act_res + 2;
  
    //
    // allocate memory
    //
	block_size_x=param->act_res / param->thread_dims[0];
	block_size_y=param->act_res / param->thread_dims[1];

    (param->u)     = (double*)calloc( sizeof(double),(block_size_x + 2)*(block_size_y + 2) );
    (param->uhelp) = (double*)calloc( sizeof(double),(block_size_x + 2)*(block_size_y + 2) );

    if( !(param->u) || !(param->uhelp) )
    {
	fprintf(stderr, "Error: Cannot allocate memory\n");
	return 0;
    }

    for( i=0; i<param->numsrcs; i++ )
    {
	/* top row */
	if(coords[1]==0)
	{
	for( j=0 ; j<block_size_x + 1 ; j++ )
	{
	    dist = sqrt( pow((double)(j + (block_size_x * coords[0]) )/ (double)(np-1) - 
			     param->heatsrcs[i].posx, 2)+
			 pow(param->heatsrcs[i].posy, 2));
	  
	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[j] +=
		    (param->heatsrcs[i].range-dist) /
		    param->heatsrcs[i].range *
		    param->heatsrcs[i].temp;
		(param->uhelp)[j] = (param->u)[j];
	    }
	}
 	}	
	/* bottom row */
	if(coords[1]==thread_dims[1]-1)
	{
	for( j=0; j<block_size_x + 1 ; j++ )
	{

            dist = sqrt( pow((double)(j + (block_size_x * coords[0]) )/ (double)(np-1) -
                             param->heatsrcs[i].posx, 2)+
                         pow(1-param->heatsrcs[i].posy, 2));

	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[(block_size_y+1)*(block_size_x+2) + j] +=
		    (param->heatsrcs[i].range-dist) / 
		    param->heatsrcs[i].range * 
		    param->heatsrcs[i].temp;
		(param->uhelp)[(block_size_y+1)*(block_size_x+2)+j] = (param->u)[(block_size_y+1)*(block_size_x+2)+j];
	    }
	}
      }

	if(coords[0]==0) 
	{
	/* leftmost column */
	for( j=0; j<block_size_y+1; j++ )
	{
	    dist = sqrt( pow(param->heatsrcs[i].posx, 2)+
			 pow((double)(j + (block_size_y * coords[1]) ) / (double)(np-1) - 
			     param->heatsrcs[i].posy, 2)); 
	  
	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[ j*(block_size_x+2) ]+=
		    (param->heatsrcs[i].range-dist) / 
		    param->heatsrcs[i].range *
		    param->heatsrcs[i].temp;
		(param->uhelp)[ j*(block_size_x+2) ] = (param->u)[ j*(block_size_x+2) ];
	    }
	}
      }

	if(coords[0]==1)
	{	
	/* rightmost column */
	for( j=1; j<block_size_y+1; j++ )
	{
	 
            dist = sqrt( pow(1 - param->heatsrcs[i].posx, 2)+
                         pow((double)(j + (block_size_y * coords[1]) ) / (double)(np-1) -  
                             param->heatsrcs[i].posy, 2));
 
	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[ j*(block_size_x+2)+(block_size_x+1) ]+=
		    (param->heatsrcs[i].range-dist) /
		    param->heatsrcs[i].range *
		    param->heatsrcs[i].temp;
		(param->uhelp)[ j*(block_size_x+2)+(block_size_x+1) ] = (param->u)[ j*(block_size_x+2)+(block_size_x+1) ];
	    }
	}
	}
    }

    return 1;
}

/*
 * free used memory
 */
int finalize( algoparam_t *param )
{
    if( param->u ) {
	free(param->u);
	param->u = 0;
    }

    if( param->uhelp ) {
	free(param->uhelp);
	param->uhelp = 0;
    }

    return 1;
}

