SUBDIRS = \
        01_message_queue_driver \
        02_nonblocking_poll_driver \
        03_ioctl_interface \
        04_kernel_timer_driver \
        05_kernel_workqueue_driver \
        06_kernel_wq_poll_driver \
	07_BBB_GPIO_Driver

.PHONY: all host target clean

all: host

# ===== HOST BUILD =====
host:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir host; \
	done

# ===== TARGET BUILD =====
target:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir target; \
	done

# ===== CLEAN =====
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
