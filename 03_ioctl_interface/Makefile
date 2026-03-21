obj-m += 03_ioctl_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

USER_APPS = 03_ioctl_test

all: modules user

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

user:
	gcc -o 03_ioctl_test 03_ioctl_test.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(USER_APPS)

load:
	sudo insmod 03_ioctl_driver.ko

unload:
	sudo rmmod 03_ioctl_driver

log:
	dmesg | tail -20
