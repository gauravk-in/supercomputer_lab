#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include "result.h"

#define FNAME_CMDARG 1

struct data_latency data_l;
char *filename;

int main (int argc, char **argv)
{
	int PING_PONG_LIMIT=5;
	int my_rank,partner_rank,i,j, nprocs;
	int tag=10;
	char a, b;
	double tstart, tend;
	MPI_Status status;

	filename = argv[FNAME_CMDARG];

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	for(i=1; i<nprocs; i++)
	{
		int ping_pong_count=0;

		MPI_Barrier(MPI_COMM_WORLD);

		if (my_rank == 0) {
			partner_rank=i;
			tstart = MPI_Wtime();
			while (ping_pong_count < PING_PONG_LIMIT) {
				// Increment the ping pong count before you send it
				ping_pong_count++;
				MPI_Send(&ping_pong_count,1 , MPI_INT, partner_rank, 10, MPI_COMM_WORLD);
				//printf("%d sent and incremented ping_pong_count %d to %d\n",my_rank, ping_pong_count, partner_rank);
				MPI_Recv(&ping_pong_count,1, MPI_INT, partner_rank, 10, MPI_COMM_WORLD, &status);
				//printf("%d received ping_pong_count %d from %d\n", my_rank, ping_pong_count, partner_rank);
			}
			tend = MPI_Wtime();

		} 
		else if(my_rank==i) {
			while (ping_pong_count < PING_PONG_LIMIT) {
				MPI_Recv(&ping_pong_count,1 , MPI_INT,0,10, MPI_COMM_WORLD, &status);
				//printf("%d received ping_pong_count %d from %d\n",my_rank, ping_pong_count, 0);
				// Increment the ping pong count before you send it
				ping_pong_count++;
				MPI_Send(&ping_pong_count,1, MPI_INT,0,10, MPI_COMM_WORLD);
				//printf("%d sent and incremented ping_pong_count %d to %d\n",my_rank, ping_pong_count, 0);
			}

		}

		MPI_Barrier(MPI_COMM_WORLD);
		if(my_rank==0){
			data_l.source_thread = 0;
			data_l.dest_thread = partner_rank;
			data_l.latency = (tend - tstart)/10;
			submit_latency_data(data_l);

			printf("Latency for process %d = %lf\n",partner_rank, (tend-tstart)/10);
		}
	}
	MPI_Finalize();
	return 0;
}
