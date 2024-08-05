all : numabench_exec run_exec testfile

numabench_exec: numabench_exec.c 
	gcc numabench_exec.c -o numabench_exec -lnuma

run_exec: run_exec.c
	gcc run_exec.c -o run_exec -lnuma

testfile:
	dd if=/dev/urandom of=testfile bs=1M count=3072

clean:
	rm -f ${OBJ} testfile numabench_exec run_exec