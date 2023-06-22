KERNELDIR = ~/linux/

obj-m += dump_numa_balancing.o

all:
	make -C $(KERNELDIR) M=$$PWD modules

clean:
	make -C $(KERNELDIR) M=$$PWD clean
