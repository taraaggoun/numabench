SRC = numabench_exec.c
OBJ = $(SRC:.c=.o)
LDFLAGS = -lnuma

all: numabench testfile

.c.o:
	${CC} -Wall -Wextra -c ${CFLAGS} $<

numabench: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

testfile:
	dd if=/dev/urandom of=testfile bs=1M count=200

clean:
	rm -f ${OBJ} testfile numabench
