CC = mpicc
CFLAGS = -O3 

all: ping-pong reduction

ping-pong: ping-pong.o result.o
	$(CC) $(CFLAGS) -o $@ $+

reduction: reduction.o
	$(CC) $(CFLAGS) -o $@ $+

clean: 
	rm -rf *.o reduction ping-pong
