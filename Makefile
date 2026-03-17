obj-m += 03_ioctl_driver.o

CC = gcc
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# userspace applications
APPS := 02_nbread 02_polltest 03_ioctl_test

all: modules $(APPS)

modules:
	make -C $(KDIR) M=$(PWD) modules

02_nbread: 02_nbread.c
	$(CC) -o $@ $

02_polltest: 02_polltest.c
	$(CC) -o $@ $

03_ioctl_test: 03_ioctl_test.c
	$(CC) -o $@ $

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f $(APPS)