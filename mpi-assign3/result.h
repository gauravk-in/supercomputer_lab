#ifndef RESULT_H
#define RESULT_H

struct data_latency {
	int source_thread;
	int dest_thread;
	double latency;
};

struct data_bandwidth {
	int source_thread;
	int dest_thread;
        int log_size;
	double time;
	double bandwidth;
};

int submit_latency_data(struct data_latency data_l);

int submit_bandwidth_data(struct data_bandwidth data_b);

int write_hello();

#endif
