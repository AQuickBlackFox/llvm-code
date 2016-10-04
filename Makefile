CC=g++
CFLAGS=-g -O3
LLVM=`llvm-config --cxxflags --ldflags --system-libs --libs core`

start:
	$(CC) $(CFLAGS) start.cpp $(LLVM) -o start

all:
	start

clean:
	rm start
