CC = mpicc
CFLAGS = -g -Wall -fopenmp
LM = -lm
OBJECTS = parallel_convolution.o utils.o
EX = filter

$(EX): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EX) $(LM)

parallel_convolution.o: parallel_convolution.c
	$(CC) $(CFLAGS) -c parallel_convolution.c

utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

.PHONY: clean debug

clean:
	rm -f $(EX) $(OBJECTS)

debug:
	mpiexec -n 4 valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all --vgdb-error=1  ./filter
