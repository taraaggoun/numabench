SRC = numabench.c
OBJ = $(SRC:.c=.o)
LDFLAGS = -lnuma

KERNELDIR_LKP ?= ~/linux-6.6.21
obj-m += openctl.o

all: numabench testfile

.c.o:
	${CC} -Wall -Wextra -c ${CFLAGS} $<

numabench: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

testfile:
	dd if=/dev/urandom of=testfile bs=1M count=200

openctl:
	make -C $(KERNELDIR_LKP) M=$(PWD) modules

clean:
	rm -f ${OBJ} testfile numabench
