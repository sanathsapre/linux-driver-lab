obj-m += 07_LEB_GPIO_Ctrl.o

PWD  := $(shell pwd)

# ===== HOST =====
KDIR_HOST := /lib/modules/$(shell uname -r)/build

# ===== TARGET =====
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-
KDIR_TARGET := /home/sanath/prep/kernel_dev/bbb/ldd/source/linux-5.10.168-ti-r76


.PHONY: all target host clean

all: target

host: target

# ===== TARGET BUILD =====
target:
	$(MAKE) -C $(KDIR_TARGET) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

# ===== CLEAN =====
clean:
	$(MAKE) -C $(KDIR_HOST) M=$(PWD) clean
	$(MAKE) -C $(KDIR_TARGET) M=$(PWD) clean
	rm -f $(USER_APPS)
