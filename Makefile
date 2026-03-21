SUBDIRS = \
	01_message_queue_driver \
	02_nonblocking_poll_driver \
	03_ioctl_interface \
	04_kernel_timer_driver

.PHONY: all clean load unload log

all:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

load:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir load; \
	done

# reverse order unload (important)
unload:
	for dir in $(shell echo $(SUBDIRS) | tr ' ' '\n' | tac); do \
		$(MAKE) -C $$dir unload; \
	done

log:
	dmesg | tail -50
