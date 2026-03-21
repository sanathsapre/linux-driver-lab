obj-m += 06_wq_poll_driver.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

USER_APPS = 06_wq_test 06_poll_test 06_nbread

all: modules user

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

user:
	gcc -o 06_wq_test 06_wq_test.c
	gcc -o 06_poll_test 06_poll_test.c
	gcc -o 06_nbread 06_nbread.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(USER_APPS)
