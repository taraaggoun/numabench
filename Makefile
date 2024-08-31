.PHONY: all clean numabench clean_numabench ioctl clean_ioctl fio clean_fio grep clean_grep

all: numabench ioctl fio grep

clean: clean_numabench clean_ioctl clean_fio clean_grep

numabench: 
	$(MAKE) -C numabench

clean_numabench: 
	$(MAKE) clean -C numabench

ioctl:
	$(MAKE) -C ioctl

clean_ioctl:
	$(MAKE) clean -C ioctl

fio:
	$(MAKE) -C fio

clean_fio:
	$(MAKE) clean -C fio

grep:
	$(MAKE) -C grep

clean_grep:
	$(MAKE) clean -C grep