CC = g++
CFLAGS = -std=c++11 -Wall

all: distvec linkstate

distvec: distvec.o
	$(CC) $(CFLAGS) distvec.o -o distvec

distvec.o: distvec.cc
	$(CC) $(CFLAGS) -c distvec.cc

linkstate: linkstate.o
	$(CC) $(CFLAGS) linkstate.o -o linkstate

linkstate.o: linkstate.cc
	$(CC) $(CFLAGS) -c linkstate.cc

clean:
	rm -rf *.o distvec linkstate