/*
 * relax_gauss.c
 *
 * Gauss-Seidel Relaxation
 *
 */

#include "heat.h"


/*
 * One Gauss-Seidel iteration step
 *
 * Flop count in inner body is 7
 */

int get_rank_from_2dcoords(int coord_x, int coord_y, int *thread_dims, MPI_Comm comm2d)
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
double relax_gauss_return_residual_2d( algoparam_t *param, int interleaving_count, int *coords, MPI_Comm comm2d)
{
    unsigned i, j, k;
	double tmp, diff, local_residual, global_residual;
	int size_x, size_y;
	int rank_left, rank_right, rank_top, rank_bottom;
	MPI_Status status;
	
	rank_left = get_rank_from_2dcoords(coords[0]-1, coords[1], param->thread_dims, comm2d);
	rank_right = get_rank_from_2dcoords(coords[0]+1, coords[1], param->thread_dims, comm2d);
	rank_top = get_rank_from_2dcoords(coords[0], coords[1]-1, param->thread_dims, comm2d);
	rank_bottom = get_rank_from_2dcoords(coords[0], coords[1]+1, param->thread_dims, comm2d);
	
	size_x = param->blocksize_x + 2;
	size_y = param->blocksize_y + 2;


	for (k = 0; k < interleaving_count; k++)
	{
		local_residual = 0;
		// Receive border values from left block
		if (rank_left != -1)
		{
			MPI_Recv(param->recbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &status);
			for (i=1; i < size_y-1; i++)
				param->u[i*size_x] = param->recbuf_left[i-1];
		}
		// Receive border values from top block
		if (rank_top != -1)
		{						
			MPI_Recv(&(param->u[1]), size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d, &status);
		}

		for( i=1; i < size_y-1; i++ )
		{
			for( j=1; j<size_x-1; j++ )
			{
				tmp = param->u[i*size_x+j];
				param->u[i*size_x+j]= 0.25 * (param->u[ i*size_x     + (j-1) ]+
						  param->u[ i*size_x     + (j+1) ]+
						  param->u[ (i-1)*size_x + j     ]+
						  param->u[ (i+1)*size_x + j     ]);
				diff = param->u[i*size_x+j] - tmp;
				local_residual += diff*diff;
			}
		}

		// Send border values to right block
		if (rank_right != -1)
		{
			for (i=1; i < size_y-1; i++)
				param->sendbuf_right[i-1] = param->u[i*size_x + size_x-2];
			MPI_Send(param->sendbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d);
		}
		// Send border values to bottom block
		if (rank_bottom != -1)
		{
			MPI_Send(&(param->u[(size_y-2)*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d);
		}
		// Send border values to left block
		if (rank_left != -1)
		{
			for (i=1; i < size_y-1; i++)
				param->sendbuf_left[i-1] = param->u[i*size_x + 1];
			MPI_Send(param->sendbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d);
		}
		// Send border values to top block
		if (rank_top != -1)
		{
			MPI_Send(&(param->u[1*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d);
		}

		// Receive border values from right block
		if (rank_right != -1)
		{
			MPI_Recv(param->recbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &status);
			for (i=1; i < size_y-1; i++)
				param->u[i*size_x+ size_x-1] = param->recbuf_right[i-1];
		}
		// Receive border values from bottom block
		if (rank_bottom != -1)
		{						
			MPI_Recv(&(param->u[(size_y-1)*size_x + 1]), size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d, &status);
		}
		
	}

	MPI_Reduce(&local_residual, &global_residual, 1, MPI_DOUBLE, MPI_SUM, 0, comm2d);
	MPI_Bcast(&global_residual, 1, MPI_DOUBLE, 0, comm2d);
	return global_residual;
}

