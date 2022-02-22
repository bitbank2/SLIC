CFLAGS=-c -Wall -O3

all: slic_conv

slic_conv: slic_conv.o
	$(CC) slic_conv.o -o slic_conv

slic_conv.o: slic_conv.c ../../src/slic.h ../../src/slic.inl
	$(CC) $(CFLAGS) slic_conv.c

clean:
	rm -rf *.o slic_conv

