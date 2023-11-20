CC = gcc 	
CFLAGS = -g -pedantic -Wall -std=gnu99 -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a4

all: crackclient crackserver

crackclient: crackclient.c
crackserver: crackserver.c

clean:
	rm -f crackserver crackclient
