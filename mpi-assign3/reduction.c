#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

main (int argc,char *argv[])
{
	int myid, np, ierr;
	int a, tmp, i,tag;
	MPI_Status status;

	ierr = MPI_Init (&argc, &argv);
	if (ierr != MPI_SUCCESS)
	{
		printf("Cannot initialize MPI!\n");
		MPI_Finalize();
		exit(0);
	}

	MPI_Comm_size(MPI_COMM_WORLD, &np);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	// ToDo: Check if np is a power of 2

	a = myid;
	i = 1;
	tag = 1; // tag can be an arbitrary value

	while (i < np)
	{
		if (myid%(2*i) == 0)
		{
		    MPI_Recv(&tmp, 1, MPI_INT, myid+i, tag, MPI_COMM_WORLD, &status);
		    a = a + tmp;
		}
		if (myid%(2*i) == i)
		{
		    MPI_Send(&a, 1, MPI_INT, myid-i, tag, MPI_COMM_WORLD);
		}
		i = i*2;
		MPI_Barrier( MPI_COMM_WORLD); // may not be required
	}

	if (myid == 0)
	{
		printf("Number of Threads = %d\n", np);
		printf("Result = %d\n", a);	
	}

	MPI_Finalize();
}
