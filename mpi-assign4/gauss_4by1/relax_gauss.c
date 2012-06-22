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
	int rank_left, rank_right;
	MPI_Status status;
	MPI_Request recrq_right, recrq_left, sendrq_left, sendrq_right;

	rank_left = get_rank_from_2dcoords(coords[0]-1, coords[1], param->thread_dims, comm2d);
	rank_right = get_rank_from_2dcoords(coords[0]+1, coords[1], param->thread_dims, comm2d);

	size_x = param->blocksize_x + 2;
	size_y = param->blocksize_y + 2;


	for (k = 0; k < interleaving_count; k++)
	{
		local_residual = 0;

		// >>> Calculation of the left border
		// Receive border values from left block
		if (rank_left != -1)
		{
			if (k==0)
				MPI_Irecv(param->recbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &recrq_left);
			MPI_Wait(&recrq_left, &status);
			for (i=1; i < size_y-1; i++)
				param->u[i*size_x] = param->recbuf_left[i-1];
                        if (k < interleaving_count-1)
                                MPI_Irecv(param->recbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &recrq_left);

		}

		// do the calculation for the left part
		j=1;
		for( i=1; i<size_y-1; i++ )
		{
			tmp = param->u[i*size_x+j];
			param->u[i*size_x+j]= 0.25 * (param->u[ i*size_x     + (j-1) ]+
					param->u[ i*size_x     + (j+1) ]+
					param->u[ (i-1)*size_x + j     ]+
					param->u[ (i+1)*size_x + j     ]);
			diff = param->u[i*size_x+j] - tmp;
			local_residual += diff*diff;
		}
		// <<< Calculation of left part

		// >>> Send the left column to the left thread for next iteration
		// Send border values to left block
		if (rank_left != -1)
		{
                        if (k>0)
                                MPI_Wait(&sendrq_left, &status);

			for (i=1; i < size_y-1; i++)
				param->sendbuf_left[i-1] = param->u[i*size_x + 1];
                        MPI_Isend(param->sendbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &sendrq_left);
		}
		// <<< Send the left column to the left thread for next iteration

		if (k > 0)
		{
			// Receive border values from right block
			if (rank_right != -1)
			{
				MPI_Irecv(param->recbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &recrq_right);
			}
		}

		// do the calculation for the inner part
		for( i=1; i < size_y-1; i++ )
		{
			for( j=2; j<size_x-2; j++ )
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

		if (k > 0)
		{
			// Receive border values from right block
			if (rank_right != -1)
			{
				MPI_Wait(&recrq_right, &status);
				for (i=1; i < size_y-1; i++)
					param->u[i*size_x+ size_x-1] = param->recbuf_right[i-1];
			}
		}

		// do the calculation for the right border
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

		// Send border values to right block
		if (rank_right != -1)
		{
			if(k!=0)
				MPI_Wait(&sendrq_right, &status);
			for (i=1; i < size_y-1; i++)
				param->sendbuf_right[i-1] = param->u[i*size_x + size_x-2];
			MPI_Isend(param->sendbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &sendrq_right);
		}

		if (k == interleaving_count-1)
		{
			// Receive border values from right block
			if (rank_right != -1)
			{
				MPI_Recv(param->recbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &status);
				for (i=1; i < size_y-1; i++)
					param->u[i*size_x+ size_x-1] = param->recbuf_right[i-1];
			}
		}
	}

	MPI_Reduce(&local_residual, &global_residual, 1, MPI_DOUBLE, MPI_SUM, 0, comm2d);
	MPI_Bcast(&global_residual, 1, MPI_DOUBLE, 0, comm2d);
	return global_residual;
}
