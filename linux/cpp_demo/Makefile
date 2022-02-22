CFLAGS=-c -Wall -O3

all: slic_test

slic_test: main.o slic.o
	$(CXX) main.o slic.o -o slic_test

main.o: main.cpp ../../src/slic.h ../../src/slic.inl
	$(CXX) $(CFLAGS) main.cpp

slic.o: ../../src/slic.cpp
	$(CXX) $(CFLAGS) ../../src/slic.cpp

clean:
	rm -rf *.o slic_test

