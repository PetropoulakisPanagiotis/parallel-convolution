CC = mpicc
CFLAGS = -g -Wall -fopenmp
LM = -lm
OBJECTS = parallel_convolution.o utils.o
EX = convolution

$(EX): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EX) $(LM)

parallel_convolution.o: parallel_convolution.c
	$(CC) $(CFLAGS) -c parallel_convolution.c

utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

.PHONY: clean debug

clean:
	rm -f $(EX) $(OBJECTS)
