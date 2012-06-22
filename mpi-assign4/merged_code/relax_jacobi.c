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

	// do the calculation for all 4 borders
		i= 1;
		for( j=1; j<size_x-1; j++ )
		{
			param->uhelp[i*size_x + j]= 0.25 * (param->u[ i*size_x+j -1 ]+  // left
						param->u[ i*size_x+j +1 ]+  // right
						param->u[ (i-1)*size_x + j ]+  // top
						param->u[ (i+1)*size_x + j ]); // bottom

				diff = param->uhelp[i*size_x + j] - param->u[i*size_x +j];
				local_residual += diff * diff;
		}
		j = 1;
		for (i = 2; i<size_y-2; i++ )
		{
			param->uhelp[i*size_x + j]= 0.25 * (param->u[ i*size_x+j -1 ]+  // left
						param->u[ i*size_x+j +1 ]+  // right
						param->u[ (i-1)*size_x + j ]+  // top
						param->u[ (i+1)*size_x + j ]); // bottom

				diff = param->uhelp[i*size_x + j] - param->u[i*size_x +j];
				local_residual += diff * diff;
		}	
		
		i = size_y-2;
		for( j=1; j<size_x-1; j++ )
		{
			param->uhelp[i*size_x + j]= 0.25 * (param->u[ i*size_x+j -1 ]+  // left
						param->u[ i*size_x+j +1 ]+  // right
						param->u[ (i-1)*size_x + j ]+  // top
						param->u[ (i+1)*size_x + j ]); // bottom

				diff = param->uhelp[i*size_x + j] - param->u[i*size_x +j];
				local_residual += diff * diff;
		}
		j = size_x-2;
		for (i = 2; i<size_y-2; i++ )
		{
			param->uhelp[i*size_x + j]= 0.25 * (param->u[ i*size_x+j -1 ]+  // left
						param->u[ i*size_x+j +1 ]+  // right
						param->u[ (i-1)*size_x + j ]+  // top
						param->u[ (i+1)*size_x + j ]); // bottom

				diff = param->uhelp[i*size_x + j] - param->u[i*size_x +j];
				local_residual += diff * diff;
		}


	// Send border values to right neighbor
		if (rank_right != -1)
		{		
			for (i=1; i < size_y-1; i++)
				param->sendbuf_right[i-1] = param->uhelp[i*size_x + size_x-2];
			MPI_Isend(param->sendbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &sendrq_right);
			
		}
	// Send border values to bottom neighbor
		if (rank_bottom != -1)
		{
			for (j=1; j < size_x -1; j++)
				param->sendbuf_bottom[j-1] = param->uhelp[(size_y-2)*size_x + j];
			MPI_Isend(param->sendbuf_bottom, size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d, &sendrq_bottom);
		}
	// Send border values to left neighbor
		if (rank_left != -1)
		{
			for (i=1; i < size_y-1; i++)
				param->sendbuf_left[i-1] = param->uhelp[i*size_x + 1];
			MPI_Isend(param->sendbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &sendrq_left);
		}
	// Send border values to top neighbor
		if (rank_top != -1)
		{
			for (j=1; j < size_x-1; j++)
				param->sendbuf_top[j-1] = param->uhelp[1*size_x + j];
			MPI_Isend(param->sendbuf_top, size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d, &sendrq_top);	
		}

	// Request receive from right neighbor
		if (rank_right != -1)
		{
			MPI_Irecv(param->recbuf_right, size_y - 2, MPI_DOUBLE, rank_right, 10, comm2d, &recrq_right);
			
		}
	// Request receive from bottom neighbor
		if (rank_bottom != -1)
		{						
			MPI_Irecv(param->recbuf_bottom, size_x - 2, MPI_DOUBLE, rank_bottom, 10, comm2d, &recrq_bottom);
		}
	// Request receive from left neighbor
		if (rank_left != -1)
		{
			MPI_Irecv(param->recbuf_left, size_y - 2, MPI_DOUBLE, rank_left, 10, comm2d, &recrq_left);
			
		}
	// Request receive from top neighbor
		if (rank_top != -1)
		{						
			MPI_Irecv(param->recbuf_top, size_x - 2, MPI_DOUBLE, rank_top, 10, comm2d, &recrq_top);
		}
	
	// Calculate the inner part
		for( i=2; i<size_y-2; i++ )
		{
			for( j=2; j<size_x-2; j++ )
		
			{
				param->uhelp[i*size_x + j]= 0.25 * (param->u[ i*size_x+j -1 ]+  // left
						param->u[ i*size_x+j +1 ]+  // right
						param->u[ (i-1)*size_x + j ]+  // top
						param->u[ (i+1)*size_x + j ]); // bottom

				diff = param->uhelp[i*size_x + j] - param->u[i*size_x +j];
				local_residual += diff * diff;
			}
		
		}


	// Wait for data from right neighbor
		if (rank_right != -1)
		{
			MPI_Wait(&recrq_right, &status);
			for (i=1; i < size_y-1; i++)
				param->uhelp[i*size_x+ size_x-1] = param->recbuf_right[i-1];
		}
	// Wait for data from bottom neighbor
		if (rank_bottom != -1)
		{
			MPI_Wait(&recrq_bottom, &status);
			for (j=1; j < size_x-1; j++)
				param->uhelp[(size_y-1)*size_x+ j] = param->recbuf_bottom[j-1];
		}
	// Wait for data from left neighbor
		if (rank_left != -1)
		{
			MPI_Wait(&recrq_left, &status);
			for (i=1; i < size_y-1; i++)
				param->uhelp[i*size_x] = param->recbuf_left[i-1];
		}
	// Wait for data from top neighbor
		if (rank_top != -1)
		{
			MPI_Wait(&recrq_top, &status);
			for (j=1; j < size_x -1; j++)
				param->uhelp[j] = param->recbuf_top[j-1];
		}


	
	MPI_Reduce(&local_residual, &global_residual, 1, MPI_DOUBLE, MPI_SUM, 0, comm2d);
	MPI_Bcast(&global_residual, 1, MPI_DOUBLE, 0, comm2d);
	return global_residual;
}
