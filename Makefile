SRC = json.c numabench.c
OBJ = $(SRC:.c=.o)
LDFLAGS = -lnuma

all: numabench testfile

.c.o:
	${CC} -Wall -Wextra -c ${CFLAGS} $<

numabench: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

testfile:
	dd if=/dev/urandom of=testfile bs=1M count=200

debug: testfile
	gcc json.c numabench.c -fsanitize=leak,address -lnuma -o numabench

clean:
	rm -f ${OBJ} testfile numabench
