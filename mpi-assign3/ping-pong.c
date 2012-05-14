#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "result.h"

#define FNAME_CMDARG 1
extern char *filename;

struct data_latency data_l;

int main (int argc, char **argv)
{

	int my_proc,other_proc,i,j, nprocs;
	int tag=10;
	char a, b;
	double tstart, tend;
	MPI_Status status;

	filename = argv[FNAME_CMDARG];

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_proc);

	for(i=1; i<nprocs; i++)
	{
		other_proc=i;
		MPI_Barrier(MPI_COMM_WORLD);


		if (my_proc == 0) {

			tstart = MPI_Wtime();
			for(j=0; j<5; j++)
			{
				MPI_Send(&a, 0, MPI_DOUBLE, other_proc, 10, MPI_COMM_WORLD);
				MPI_Recv(&b, 0, MPI_DOUBLE, other_proc, 10, MPI_COMM_WORLD, &status);
			}
			tend = MPI_Wtime();

		} else {
			for(j=0; j<5; j++)
			{
				MPI_Recv(&b, 0, MPI_DOUBLE, 0, 10, MPI_COMM_WORLD, &status);

				MPI_Send(&b, 0, MPI_DOUBLE, 0, 10, MPI_COMM_WORLD);
			}

		}

		MPI_Barrier(MPI_COMM_WORLD);
		data_l.source_thread = 0;
		data_l.dest_thread = other_proc;
		data_l.latency = (tend - tstart)/10;
		submit_latency_data(data_l);
		printf("Latency for process %d = %lf\n",other_proc, tend-tstart);
	}
	MPI_Finalize();
}
