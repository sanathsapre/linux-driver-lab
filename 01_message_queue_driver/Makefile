obj-m += 01_msg_queue_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod 01_msg_queue_driver.ko

unload:
	sudo rmmod 01_msg_queue_driver

log:
	dmesg | tail -20
