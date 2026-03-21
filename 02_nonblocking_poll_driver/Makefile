obj-m += 02_poll_noblock_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

USER_APPS = 02_nbread 02_polltest

all: modules user

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

user:
	gcc -o 02_nbread 02_nbread.c
	gcc -o 02_polltest 02_polltest.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(USER_APPS)

load:
	sudo insmod 02_poll_noblock_driver.ko

unload:
	sudo rmmod 02_poll_noblock_driver

log:
	dmesg | tail -20
