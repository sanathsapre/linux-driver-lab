obj-m += 04_timer_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

USER_APPS = 04_timer_test 04_timer_read

all: modules user

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

user:
	gcc -o 04_timer_test 04_timer_test.c
	gcc -o 04_timer_read 04_timer_read.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(USER_APPS)

load:
	sudo insmod 04_timer_driver.ko

unload:
	sudo rmmod 04_timer_driver

log:
	dmesg | tail -20
