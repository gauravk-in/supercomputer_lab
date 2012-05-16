CC = mpicc
CFLAGS = -O3 

all: ping-pong reduction ping-pong-bandwidth

ping-pong: ping-pong.o result.o
	$(CC) $(CFLAGS) -o $@ $+

reduction: reduction.o
	$(CC) $(CFLAGS) -o $@ $+

ping-pong-bandwidth: ping-pong-bandwidth.o result.o
	$(CC) $(CFLAGS) -o $@ $+ -lm

clean: 
	rm -rf *.o reduction ping-pong ping-pong-bandwidth
