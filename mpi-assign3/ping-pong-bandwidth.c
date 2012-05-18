#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "result.h"

#define FNAME_CMDARG 1
#define MAX_CMDARG 2
char *filename;

struct data_bandwidth data_b;

void usage()
{
	printf("ping-pong-bandwidth <result-file.txt>\n");
	exit(0);
}

int main (int argc, char **argv)
{
	int my_proc, other_proc, i, j, nprocs, size;
	int tag=10;
	char *msg;
	char *buffer;
	double tstart, tend;
	MPI_Status status;


	if(argc > MAX_CMDARG)
		usage();
	filename = argv[FNAME_CMDARG];

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_proc);

	if(my_proc == 0)
	{
		for(i=1; i<nprocs; i++)
		{
			other_proc=i;
			for(size=0; size<=20;size++)
			{
				buffer=malloc(pow(2,size)*sizeof(char));
				msg=calloc(pow(2, size),sizeof(char));

				tstart = MPI_Wtime();   
				for(j=0; j<5; j++)
				{
					MPI_Send(msg, pow(2, size), MPI_CHAR, other_proc, 10, MPI_COMM_WORLD);
					MPI_Recv(buffer, pow(2, size), MPI_CHAR, other_proc, 10, MPI_COMM_WORLD, &status);
				}
				tend = MPI_Wtime();
				free(msg);
				free(buffer);
				data_b.source_thread = 0;
				data_b.dest_thread = other_proc;
				data_b.log_size = size;
				data_b.time = (tend - tstart)/10;
				data_b.bandwidth = 0;
				submit_bandwidth_data(data_b);
			}
		}
	}
	else 
	{
		for(size=0;size<=20;size++)
		{
			buffer=malloc(pow(2,size)*sizeof(char));

			for(j=0; j<5; j++)
			{
				MPI_Recv(buffer, pow(2,size), MPI_CHAR, 0, 10, MPI_COMM_WORLD, &status);
				MPI_Send(buffer, pow(2,size), MPI_CHAR, 0, 10, MPI_COMM_WORLD);
			}
			free(buffer);
		}
	}

	MPI_Finalize();
	return 0;
}
