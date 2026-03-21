obj-m += 05_workqueue_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

USER_APPS = 05_wq_test 05_wq_read

all: modules user

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

user:
	gcc -o 05_wq_test 05_wq_test.c
	gcc -o 05_wq_read 05_wq_read.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(USER_APPS)
