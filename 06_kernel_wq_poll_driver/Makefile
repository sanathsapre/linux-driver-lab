obj-m += 06_wq_poll_driver.o

PWD  := $(shell pwd)

# ===== HOST =====
KDIR_HOST := /lib/modules/$(shell uname -r)/build

# ===== TARGET =====
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
KDIR_TARGET := /home/sanath/prep/kernel_dev/bbb/ldd/source/linux-5.10.168-ti-r76

USER_APPS = 06_wq_test 06_poll_test 06_nbread

.PHONY: all host target modules user clean

all: host

# ===== HOST BUILD =====
host: modules user

modules:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) modules

user:
	gcc -o 06_wq_test 06_wq_test.c
	gcc -o 06_poll_test 06_poll_test.c
	gcc -o 06_nbread 06_nbread.c

# ===== TARGET BUILD =====
target:
	$(MAKE) -C $(KDIR_TARGET) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	$(CROSS_COMPILE)gcc -o 06_wq_test 06_wq_test.c
	$(CROSS_COMPILE)gcc -o 06_poll_test 06_poll_test.c
	$(CROSS_COMPILE)gcc -o 06_nbread 06_nbread.c

# ===== CLEAN =====
clean:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) clean
	$(MAKE) -C $(KDIR_TARGET) M=$(PWD) clean
	rm -f $(USER_APPS)
