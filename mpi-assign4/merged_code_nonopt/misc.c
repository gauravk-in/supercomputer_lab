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
int initialize( algoparam_t *param , int *coords)
{
    int i, j;
    double dist;

    // total number of points 
    const int np = param->act_res + 2;
  
    param->blocksize_x=param->act_res / param->thread_dims[0];
    param->blocksize_y=param->act_res / param->thread_dims[1];

	//
    // allocate memory
    //
    (param->sendbuf_left) = (double*)calloc( sizeof(double), param->blocksize_y );
    (param->recbuf_left) = (double*)calloc( sizeof(double), param->blocksize_y );
    (param->sendbuf_right) = (double*)calloc( sizeof(double), param->blocksize_y );
    (param->recbuf_right) = (double*)calloc( sizeof(double), param->blocksize_y );
    (param->sendbuf_top) = (double*)calloc( sizeof(double), param->blocksize_x );
    (param->recbuf_top) = (double*)calloc( sizeof(double), param->blocksize_x );
    (param->sendbuf_bottom) = (double*)calloc( sizeof(double), param->blocksize_x );
    (param->recbuf_bottom) = (double*)calloc( sizeof(double), param->blocksize_x );
    (param->u)     = (double*)calloc( sizeof(double),(param->blocksize_x + 2)*(param->blocksize_y + 2) );
    (param->uhelp) = (double*)calloc( sizeof(double),(param->blocksize_x + 2)*(param->blocksize_y + 2) );

    if( !(param->u) || !(param->uhelp) || !(param->sendbuf_left) || !(param->sendbuf_right) || !(param->recbuf_left) || !(param->recbuf_right) )
    {
	fprintf(stderr, "Error: Cannot allocate memory\n");
	return 0;
    }

    for( i=0; i<param->numsrcs; i++ )
    {
	/* top row */
	if(coords[1]==0)
	{
	for( j=0 ; j<param->blocksize_x + 1 ; j++ )
	{
	    dist = sqrt( pow((double)(j + (param->blocksize_x * coords[0]) )/ (double)(np-1) - 
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
	if(coords[1]==param->thread_dims[1]-1)
	{
	for( j=0; j<param->blocksize_x + 1 ; j++ )
	{

            dist = sqrt( pow((double)(j + (param->blocksize_x * coords[0]) )/ (double)(np-1) -
                             param->heatsrcs[i].posx, 2)+
                         pow(1-param->heatsrcs[i].posy, 2));

	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[(param->blocksize_y+1)*(param->blocksize_x+2) + j] +=
		    (param->heatsrcs[i].range-dist) / 
		    param->heatsrcs[i].range * 
		    param->heatsrcs[i].temp;
		(param->uhelp)[(param->blocksize_y+1)*(param->blocksize_x+2)+j] = (param->u)[(param->blocksize_y+1)*(param->blocksize_x+2)+j];
	    }
	}
      }

	if(coords[0]==0) 
	{
	/* leftmost column */
	for( j=0; j<param->blocksize_y+1; j++ )
	{
	    dist = sqrt( pow(param->heatsrcs[i].posx, 2)+
			 pow((double)(j + (param->blocksize_y * coords[1]) ) / (double)(np-1) - 
			     param->heatsrcs[i].posy, 2)); 
	  
	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[ j*(param->blocksize_x+2) ]+=
		    (param->heatsrcs[i].range-dist) / 
		    param->heatsrcs[i].range *
		    param->heatsrcs[i].temp;
		(param->uhelp)[ j*(param->blocksize_x+2) ] = (param->u)[ j*(param->blocksize_x+2) ];
	    }
	}
      }

	if(coords[0]==param->thread_dims[0]-1)
	{	
	/* rightmost column */
	for( j=1; j<param->blocksize_y+1; j++ )
	{
	 
            dist = sqrt( pow(1 - param->heatsrcs[i].posx, 2)+
                         pow((double)(j + (param->blocksize_y * coords[1]) ) / (double)(np-1) -  
                             param->heatsrcs[i].posy, 2));
 
	    if( dist <= param->heatsrcs[i].range )
	    {
		(param->u)[ j*(param->blocksize_x+2)+(param->blocksize_x+1) ]+=
		    (param->heatsrcs[i].range-dist) /
		    param->heatsrcs[i].range *
		    param->heatsrcs[i].temp;
		(param->uhelp)[ j*(param->blocksize_x+2)+(param->blocksize_x+1) ] = (param->u)[ j*(param->blocksize_x+2)+(param->blocksize_x+1) ];
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
    if( param->u ) 
	{
		free(param->u);
		param->u = 0;
    }

    if( param->uhelp ) 
	{
		free(param->uhelp);
		param->uhelp = 0;
    }

	if ( param->sendbuf_left ) 
	{
		free(param->sendbuf_left);
		param->sendbuf_left = 0;
	}

	if ( param->sendbuf_right ) 
	{
		free(param->sendbuf_right);
		param->sendbuf_right = 0;
	}

	if ( param->recbuf_left ) 
	{
		free(param->recbuf_left);
		param->recbuf_left = 0;
	}

	if ( param->recbuf_right ) 
	{
		free(param->recbuf_right);
		param->recbuf_right = 0;
	}

	if ( param->sendbuf_top ) 
	{
		free(param->sendbuf_top);
		param->sendbuf_top = 0;
	}

	if ( param->sendbuf_bottom ) 
	{
		free(param->sendbuf_bottom);
		param->sendbuf_bottom = 0;
	}

	if ( param->recbuf_top ) 
	{
		free(param->recbuf_top);
		param->recbuf_top = 0;
	}

	if ( param->recbuf_bottom ) 
	{
		free(param->recbuf_bottom);
		param->recbuf_bottom = 0;
	}

    return 1;
}

