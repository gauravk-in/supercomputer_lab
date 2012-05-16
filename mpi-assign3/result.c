#include <stdio.h>
#include "result.h"

extern char *filename;

int submit_bandwidth_data(struct data_bandwidth data_b)
{
	FILE* fp;
	fp = fopen(filename, "a");

	fprintf(fp, "%d\t%d\t%d\t%lf\t%lf\n", data_b.source_thread,
			data_b.dest_thread, data_b.log_size, 
			data_b.time, data_b.bandwidth);

	fclose(fp);
	return 1;
}

int submit_latency_data(struct data_latency data_l)
{
	FILE* fp;
	fp=fopen(filename, "a");
	
	fprintf(fp, "%d\t%d\t%lf\n", data_l.source_thread,
			data_l.dest_thread, data_l.latency);

	fclose(fp);
	return 1;
}

