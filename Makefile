CC=gcc
CFLAGS=-Wall -pedantic -std=gnu99

.PHONY: all clean
.DEFAULT_GOAL := all

all: jobrunner

jobrunner: jobrunner.c
	$(CC) $(CFLAGS) -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a3 jobrunner.c -o jobrunner 

clean:
	rm -f jobrunner

