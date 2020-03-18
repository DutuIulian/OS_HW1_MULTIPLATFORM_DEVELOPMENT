CC=gcc

all: so-cpp

build: so-cpp

so-cpp: so-cpp.o table.o
	$(CC) -o so-cpp so-cpp.o table.o

so-cpp.o: so-cpp.c
	$(CC) -c so-cpp.c
	
table.o: table.c
	$(CC) -c table.c

.PHONY: clean
clean:
	rm so-cpp *.o