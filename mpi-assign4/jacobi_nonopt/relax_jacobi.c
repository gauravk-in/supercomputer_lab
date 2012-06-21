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


/*int get_rank_from_2dcoords(int coord_x, int coord_y, int *thread_dims, MPI_Comm comm2d)
{
	// returns -1 if coordinates are invalid

	int rank;
	int coords[2] = {coord_x, coord_y};

	if (coord_x < 0) return -1;
	if (coord_y < 0) return -1;
	if (coord_x >= thread_dims[0]) return -1;
	if (coord_y >= thread_dims[1]) return -1;

	MPI_Cart_rank(comm2d, coords, &rank);
	return rank;
}
*/

/*
 * One Jacobi iteration step
 */
double relax_jacobi_return_residual( algoparam_t *param, int *coords, MPI_Comm comm2d )
{
	int i, j;
	double tmp, diff, local_residual, global_residual;
	int size_x, size_y;
	int rank_left, rank_right, rank_top, rank_bottom;
	MPI_Request sendrq_right, recrq_right, sendrq_left, recrq_left, sendrq_top, recrq_top, sendrq_bottom, recrq_bottom;
	MPI_Status status;
	
	rank_left = get_rank_from_2dcoords(coords[0]-1, coords[1], param->thread_dims, comm2d);
	rank_right = get_rank_from_2dcoords(coords[0]+1, coords[1], param->thread_dims, comm2d);
	rank_top = get_rank_from_2dcoords(coords[0], coords[1]-1, param->thread_dims, comm2d);
	rank_bottom = get_rank_from_2dcoords(coords[0], coords[1]+1, param->thread_dims, comm2d);
	local_residual=0.0;
	size_x = param->blocksize_x + 2;
	size_y = param->blocksize_y + 2;



		

	for( i=1; i<size_y-1; i++ )
	{
		for( j=1; j<size_x-1; j++ )
		
			{
				param->uhelp[i*size_x + j]= 0.25 * (param->u[ i*size_x+j -1 ]+  // left
						param->u[ i*size_x+j +1 ]+  // right
						param->u[ (i-1)*size_x + j ]+  // top
						param->u[ (i+1)*size_x + j ]); // bottom

				diff = param->uhelp[i*size_x + j] - param->u[i*size_x +j];
				local_residual += diff * diff;
			}
		
	}


	// Send border values to right block
		if (rank_right != -1)
		{
			for (i=1; i < size_y-1; i++)
				param->sendbuf_right[i-1] = param->uhelp[i*size_x + size_x-2];
			MPI_Send(param->sendbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d);
		}
		// Send border values to bottom block
		if (rank_bottom != -1)
		{
			MPI_Send(&(param->uhelp[(size_y-2)*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d);
		}
		// Send border values to left block
		if (rank_left != -1)
		{
			for (i=1; i < size_y-1; i++)
				param->sendbuf_left[i-1] = param->uhelp[i*size_x + 1];
			MPI_Send(param->sendbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d);
		}
		// Send border values to top block
		if (rank_top != -1)
		{
			MPI_Send(&(param->uhelp[1*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d);
		}

		// Receive border values from left block
		if (rank_left != -1)
		{
			MPI_Recv(param->recbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &status);
			for (i=1; i < size_y-1; i++)
				param->uhelp[i*size_x] = param->recbuf_left[i-1];
		}
		// Receive border values from top block
		if (rank_top != -1)
		{						
			MPI_Recv(&(param->uhelp[1]), size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d, &status);
		}
		// Receive border values from right block
		if (rank_right != -1)
		{
			MPI_Recv(param->recbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &status);
			for (i=1; i < size_y-1; i++)
				param->uhelp[i*size_x+ size_x-1] = param->recbuf_right[i-1];
		}
		// Receive border values from bottom block
		if (rank_bottom != -1)
		{						
			MPI_Recv(&(param->uhelp[(size_y-1)*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d, &status);
		}


	

/*
		// Send border values to right block
		if (rank_right != -1)
		{
			for (i=1; i < size_y-1; i++)
				param->sendbuf_right[i-1] = param->uhelp[i*size_x + size_x-2];
			MPI_Send(param->sendbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d);
		}
		// Send border values to bottom block
		if (rank_bottom != -1)
		{
			MPI_Send(&(param->uhelp[(size_y-2)*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d);
		}
		// Send border values to left block
		if (rank_left != -1)
		{
			for (i=1; i < size_y-1; i++)
				param->sendbuf_left[i-1] = param->uhelp[i*size_x + 1];
			MPI_Send(param->sendbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d);
		}
		// Send border values to top block
		if (rank_top != -1)
		{
			MPI_Send(&(param->uhelp[1*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d);
		}

		// Receive border values from left block
		if (rank_left != -1)
		{
			MPI_Recv(param->recbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &status);
			for (i=1; i < size_y-1; i++)
				param->uhelp[i*size_x] = param->recbuf_left[i-1];
		}
		// Receive border values from top block
		if (rank_top != -1)
		{						
			MPI_Recv(&(param->uhelp[1]), size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d, &status);
		}
		// Receive border values from right block
		if (rank_right != -1)
		{
			MPI_Recv(param->recbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &status);
			for (i=1; i < size_y-1; i++)
				param->uhelp[i*size_x+ size_x-1] = param->recbuf_right[i-1];
		}
		// Receive border values from bottom block
		if (rank_bottom != -1)
		{						
			MPI_Recv(&(param->uhelp[(size_y-1)*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d, &status);
		}
*/
	MPI_Reduce(&local_residual, &global_residual, 1, MPI_DOUBLE, MPI_SUM, 0, comm2d);
	MPI_Bcast(&global_residual, 1, MPI_DOUBLE, 0, comm2d);
	return global_residual;
}
