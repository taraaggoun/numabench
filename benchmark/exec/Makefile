SRC = numabench_exec.c
OBJ = $(SRC:.c=.o)
LDFLAGS = -lnuma

all: numabench testfile

.c.o:
	${CC} -Wall -Wextra -c ${CFLAGS} $<

numabench: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

regenerate_pagecache:
	${CC} regenerate_pagecache.c -o regenerate_pagecache -lnuma

surnumabench:
	${CC} surnumabench.c -lnuma -D_GNU_SOURCE


testfile:
	dd if=/dev/urandom of=testfile bs=1M count=200

clean:
	rm -f ${OBJ} testfile numabench
