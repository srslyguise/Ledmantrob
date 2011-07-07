CC=g++
LIBS=-lSDL -lSDL_ttf

all: ldt

ldt: ldt.o
	$(CC) -o ldt ldt.o $(LIBS)

ldt.o: ldt.cpp
	$(CC) -c -o ldt.o ldt.cpp

clean:
	rm -f ldt.o
	rm -f ldt
