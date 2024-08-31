CC = gcc
CCFLAGS = -Wall
LDFLAGS = -lnuma

all: fio

fio:
	$(CC) $(CCFLAGS) run_fio.c -o run_fio ${LDFLAGS}

clean:
	rm run_fio