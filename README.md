# Linux Driver Lab 🚀

A hands-on collection of **Linux kernel driver implementations** along with corresponding **user-space test programs**, built to master **Embedded Linux and Kernel Development**.

This repository follows a **progressive, concept-driven approach**, where each driver introduces a new kernel mechanism used in real-world systems.

---

# 📁 Repository Structure

```
linux-driver-lab/
├── 01_msg_queue_driver.c        # Basic character driver (FIFO queue)
├── 02_poll_noblock_driver.c    # Blocking + non-blocking + poll/select
├── 02_nbread.c                 # Non-blocking read test program
├── 02_polltest.c               # poll/select test program
├── 03_ioctl_driver.c           # IOCTL-enabled driver
├── 03_ioctl_test.c             # IOCTL test program
├── Makefile
└── README.md
```

---

# 🧠 Drivers Overview

---

## 🔹 01 — Message Queue Character Driver

### Features

* FIFO queue (3 messages × 128 bytes)
* `read()` / `write()` support
* Multi-process safe
* Kernel-user communication

### Concepts Covered

* `file_operations`
* `copy_to_user` / `copy_from_user`
* `cdev` registration
* Mutex-based synchronization

---

## 🔹 02 — Blocking + Non-Blocking + poll/select

### Features

* Blocking `read()` (wait queues)
* Non-blocking mode (`O_NONBLOCK`)
* `poll()` / `select()` support
* Multiple readers supported

### Concepts Covered

* Wait queues
* Sleep/wakeup mechanism
* Event-driven I/O
* `poll_wait()` usage

---

## 🔹 03 — IOCTL Control Interface

### Features

Updated FIFO queue (16 messages × 128 bytes)
Control operations exposed via `ioctl()`:

| Command          | Description                 |
| ---------------- | --------------------------- |
| GET_QUEUE_SIZE   | Returns number of messages  |
| GET_MAX_CAPACITY | Returns queue capacity (16) |
| CLEAR_QUEUE      | Clears all messages         |
| RESET_DEVICE     | Resets driver state         |

### Concepts Covered

* `unlocked_ioctl`
* Kernel-user control interface
* Extensible driver design

---

# 🧪 User-Space Test Programs

---

## 🔸 02_nbread.c

Tests **non-blocking read behavior**

```bash
./02_nbread
```

Expected:

* Immediate return when queue is empty
* `EAGAIN` handling

---

## 🔸 02_polltest.c

Tests **poll/select behavior**

```bash
./02_polltest
```

Expected:

* Waits for data
* Wakes up when message is written

---

## 🔸 03_ioctl_test.c

Tests all IOCTL commands

```bash
./03_ioctl_test
```

Expected:

* Queue size reporting
* Queue clearing
* Device reset behavior

---

# ⚙️ Build Instructions

Build kernel modules:

```bash
make
```

---

# 🚀 Load Driver

```bash
sudo insmod <driver>.ko
dmesg | tail
```

---

# 🧹 Unload Driver

```bash
sudo rmmod <driver_name>
dmesg | tail
```

---

# 📟 Device Node

Example:

```bash
ls -l /dev/sanath_queue
```

---

# 🧪 Basic Usage

### Write to device

```bash
echo "hello" > /dev/sanath_queue
```

### Read from device

```bash
cat /dev/sanath_queue
```

---

# 🧪 IOCTL Usage Example

```c
ioctl(fd, GET_QUEUE_SIZE);
ioctl(fd, CLEAR_QUEUE);
```

---

# 🧠 What This Repository Demonstrates

This project showcases:

* Real Linux driver behavior (not toy examples)
* Blocking vs non-blocking I/O
* Event-driven design (`poll/select`)
* Driver control via `ioctl`
* Safe concurrency handling

---

# 📈 Learning Roadmap

Completed:

* [x] Character driver
* [x] Wait queues
* [x] poll/select
* [x] IOCTL interface

Upcoming:

* [ ] Timer-based driver
* [ ] Workqueue driver
* [ ] GPIO driver (hardware interaction)
* [ ] Interrupt handling
* [ ] Platform driver (device tree)

---

# ⚠️ Disclaimer

These drivers are implemented for **learning and experimentation purposes** and are not production-ready.

---

# 👨‍💻 Author

Sanath Kumar P Sapre

---

# ⭐ Final Note

This repository reflects a **step-by-step journey into Linux kernel development**, focusing on building real understanding through implementation.

Each driver adds a new layer of complexity — similar to how real-world kernel subsystems evolve.