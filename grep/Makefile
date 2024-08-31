CC = gcc
CCFLAGS = -Wall
LDFLAGS = -lnuma

all: grep

grep:
	$(CC) $(CCFLAGS) run_grep.c -o run_grep ${LDFLAGS}

clean:
	rm run_grep