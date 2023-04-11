all: numabench

numabench: testfile
	gcc json.c numabench.c -lnuma -o numabench

testfile:
	dd if=/dev/urandom of=testfile bs=1M count=200

debug: testfile
	gcc json.c numabench.c -fsanitize=leak,address -lnuma -o numabench

clean:
	rm -f testfile
	rm numabench
