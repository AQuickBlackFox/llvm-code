CC=g++
CFLAGS=-g -O3
LLVM=`llvm-config --cxxflags --ldflags --system-libs --libs core`

Release:
	$(CC) $(CFLAGS) Source.cpp $(LLVM) -o run

all:
	Release

clean:
	rm run
