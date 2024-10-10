all: TCPechod TCPecho

CFLAGS=-include /usr/include/errno.h 

TCPechod: TCPechod.c errexit.c 

TCPecho: TCPecho.c errexit.c 

clean:
	rm -f *.o TCPechod TCPecho
