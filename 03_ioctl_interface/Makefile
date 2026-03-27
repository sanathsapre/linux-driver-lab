obj-m += 03_ioctl_driver.o

PWD  := $(shell pwd)

# ===== HOST =====
KDIR_HOST := /lib/modules/$(shell uname -r)/build

# ===== TARGET =====
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
KDIR_TARGET := /home/sanath/prep/kernel_dev/bbb/ldd/source/linux-5.10.168-ti-r76

USER_APPS = 03_ioctl_test

.PHONY: all host target modules user clean

all: host

# ===== HOST BUILD =====
host: modules user

modules:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) modules

user:
	gcc -o 03_ioctl_test 03_ioctl_test.c

# ===== TARGET BUILD =====
target:
	$(MAKE) -C $(KDIR_TARGET) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	$(CROSS_COMPILE)gcc -o 03_ioctl_test 03_ioctl_test.c

# ===== CLEAN =====
clean:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) clean
	$(MAKE) -C $(KDIR_TARGET) M=$(PWD) clean
	rm -f $(USER_APPS)
