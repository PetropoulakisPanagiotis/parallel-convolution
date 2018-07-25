CC = gcc
CFLAGS = -g -Wall
OBJECTS = parallel_convolution.o utils.o
EX = filter

$(EX): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EX)

parallel_convolution.o: parallel_convolution.c
	$(CC) $(CFLAGS) -c parallel_convolution.c

utils.o: utils.c
	$(CC) $(CFLAGS) -c utils.c

.PHONY: clean

clean:
	rm -f $(EX) $(OBJECTS)

