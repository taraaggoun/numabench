# Path to kernel
KERNELDIR_LKP ?= /home/tara/linux-6.6.21

obj-m += openctl.o

openctl:
	make -C $(KERNELDIR_LKP) M=$(PWD) modules

clean:
	make -C $(KERNELDIR_LKP) M=$(PWD) clean