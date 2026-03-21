SUBDIRS = \
	01_message_queue_driver \
	02_nonblocking_poll_driver \
	03_ioctl_interface \
	04_kernel_timer_driver \
	05_kernel_workqueue_driver \
	06_kernel_wq_poll_driver

.PHONY: all clean load unload log

all:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
