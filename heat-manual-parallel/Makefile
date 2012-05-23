# Intel compiler
CC =  icc 
CFLAGS = -openmp -O3

MPICC = mpicc

all: heat

heat : heat.o input.o misc.o timing.o relax_gauss.o relax_jacobi.o
	$(CC) $(CFLAGS) $(PAPI_INC) -o $@ $+ $(PAPI_LIB) -lm 

%.o : %.c %.h
	$(CC) $(CFLAGS) $(PAPI_INC) -c -o $@ $<

clean:
	rm -f *.o heat *~ *.ppm

remake : clean all
