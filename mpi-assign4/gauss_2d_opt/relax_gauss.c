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

double relax_gauss_return_residual( algoparam_t *param, int interleaving_count, int *coords, MPI_Comm comm2d)
{
    unsigned i, j, k;
	double tmp, diff, local_residual, global_residual;
	int size_x, size_y;
	int rank_left, rank_right, rank_top, rank_bottom;
	MPI_Request sendrq_right, recrq_right, sendrq_left, recrq_left, sendrq_top, recrq_top, sendrq_bottom, recrq_bottom;
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
			if (k == 0)
				MPI_Irecv(param->recbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &recrq_left);
			MPI_Wait(&recrq_left, &status);
			for (i=1; i < size_y-1; i++)
				param->u[i*size_x] = param->recbuf_left[i-1];
			if (k < interleaving_count-1)
				MPI_Irecv(param->recbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &recrq_left);
		}
		// Receive border values from top block
		if (rank_top != -1)
		{
			if (k==0)
				MPI_Irecv(param->recbuf_top, size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d, &recrq_top);					
			MPI_Wait(&recrq_top, &status);
			for (j=1; j < size_x -1; j++)
				param->u[j] = param->recbuf_top[j-1];
			if (k< interleaving_count-1)
				MPI_Irecv(param->recbuf_top, size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d, &recrq_top);	
		}

		// do the calculation for the upper left + inner part
		for( i=1; i < size_y-2; i++ )
		{
			for( j=1; j<size_x-2; j++ )
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

		// Receive border values from right block
		if (rank_right != -1)
		{
			if (k>0)
			{
				MPI_Wait(&recrq_right, &status);
				for (i=1; i < size_y-1; i++)
					param->u[i*size_x+ size_x-1] = param->recbuf_right[i-1];
			}			
			MPI_Irecv(param->recbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &recrq_right);
		}
		// Receive border values from bottom block
		if (rank_bottom != -1)
		{	
			if (k>0)
			{
				MPI_Wait(&recrq_bottom, &status);
				for (j=1; j < size_x-1; j++)
					param->u[(size_y-1)*size_x+ j] = param->recbuf_bottom[j-1];
			}				
			MPI_Irecv(param->recbuf_bottom, size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d, &recrq_bottom);
		}

		// do the calculation for the lower right border
		i = size_y-2;
		for( j=1; j<size_x-2; j++ )
		{
			tmp = param->u[i*size_x+j];
			param->u[i*size_x+j]= 0.25 * (param->u[ i*size_x     + (j-1) ]+
					  param->u[ i*size_x     + (j+1) ]+
					  param->u[ (i-1)*size_x + j     ]+
					  param->u[ (i+1)*size_x + j     ]);
			diff = param->u[i*size_x+j] - tmp;
			local_residual += diff*diff;
		}
		j = size_x-2;
		for (i = 1; i<size_y-1; i++ )
		{
			tmp = param->u[i*size_x+j];
			param->u[i*size_x+j]= 0.25 * (param->u[ i*size_x     + (j-1) ]+
					  param->u[ i*size_x     + (j+1) ]+
					  param->u[ (i-1)*size_x + j     ]+
					  param->u[ (i+1)*size_x + j     ]);
			diff = param->u[i*size_x+j] - tmp;
			local_residual += diff*diff;
		}

		// Send border values to left block
		if (rank_left != -1)
		{
			if (k>0)
				MPI_Wait(&sendrq_left, &status);
				
			for (i=1; i < size_y-1; i++)
				param->sendbuf_left[i-1] = param->u[i*size_x + 1];
			MPI_Isend(param->sendbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &sendrq_left);
			
		}
		// Send border values to top block
		if (rank_top != -1)
		{
			if (k>0)
				MPI_Wait(&sendrq_top, &status);
			
			for (j=1; j < size_x-1; j++)
				param->sendbuf_top[j-1] = param->u[1*size_x + j];
			MPI_Isend(param->sendbuf_top, size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d, &sendrq_top);			
		}

		// Send border values to right block
		if (rank_right != -1)
		{
			if (k>0)
				MPI_Wait(&sendrq_right, &status);		
			for (i=1; i < size_y-1; i++)
				param->sendbuf_right[i-1] = param->u[i*size_x + size_x-2];
			MPI_Isend(param->sendbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &sendrq_right);
			
		}
		// Send border values to bottom block
		if (rank_bottom != -1)
		{
			if (k>0)
				MPI_Wait(&sendrq_bottom, &status);
			for (j=1; j < size_x -1; j++)
				param->sendbuf_bottom[j-1] = param->u[(size_y-2)*size_x + j];
			MPI_Isend(param->sendbuf_bottom, size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d, &sendrq_bottom);
		}	
		
		if (k == interleaving_count-1)
		{
			// Receive border values from right block
			if (rank_right != -1)
			{
				MPI_Wait(&recrq_right, &status);
				for (i=1; i < size_y-1; i++)
					param->u[i*size_x+ size_x-1] = param->recbuf_right[i-1];	
			}
			// Receive border values from bottom block
			if (rank_bottom != -1)
			{		
				MPI_Wait(&recrq_bottom, &status);
				for (j=1; j < size_x-1; j++)
					param->u[(size_y-1)*size_x+ j] = param->recbuf_bottom[j-1];
			}
		}
	}

	MPI_Reduce(&local_residual, &global_residual, 1, MPI_DOUBLE, MPI_SUM, 0, comm2d);
	MPI_Bcast(&global_residual, 1, MPI_DOUBLE, 0, comm2d);
	return global_residual;
}
