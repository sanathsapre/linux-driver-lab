obj-m += 04_timer_driver.o

PWD  := $(shell pwd)

# ===== HOST =====
KDIR_HOST := /lib/modules/$(shell uname -r)/build

# ===== TARGET =====
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
KDIR_TARGET := /home/sanath/prep/kernel_dev/bbb/ldd/source/linux-5.10.168-ti-r76

USER_APPS = 04_timer_test 04_timer_read

.PHONY: all host target modules user clean

all: host

# ===== HOST BUILD =====
host: modules user

modules:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) modules

user:
	gcc -o 04_timer_test 04_timer_test.c
	gcc -o 04_timer_read 04_timer_read.c

# ===== TARGET BUILD =====
target:
	$(MAKE) -C $(KDIR_TARGET) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	$(CROSS_COMPILE)gcc -o 04_timer_test 04_timer_test.c
	$(CROSS_COMPILE)gcc -o 04_timer_read 04_timer_read.c

# ===== CLEAN =====
clean:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) clean
	$(MAKE) -C $(KDIR_TARGET) M=$(PWD) clean
	rm -f $(USER_APPS)
