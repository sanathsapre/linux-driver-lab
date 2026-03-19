# Linux Driver Lab 🚀

A hands-on collection of **Linux kernel driver implementations** along with corresponding **user-space test programs**, built to master **Embedded Linux and Kernel Development**.

This repository follows a **progressive, concept-driven approach**, where each driver introduces a new kernel mechanism used in real-world systems.

---

# 📁 Repository Structure

```bash
linux-driver-lab/
├── 01_msg_queue_driver.c        # Basic character driver (FIFO queue)
├── 02_poll_noblock_driver.c    # Blocking + non-blocking + poll/select
├── 02_nbread.c                 # Non-blocking read test program
├── 02_polltest.c               # poll/select test program
├── 03_ioctl_driver.c           # IOCTL-enabled driver
├── 03_ioctl_test.c             # IOCTL test program
├── 04_timer_driver.c           # Timer-based event driver
├── Lessons_Learnt/
│   └── 04_timer_learnings.html # Notes & learnings for timer driver
├── Makefile
└── README.md
```

---

# 🧠 Drivers Overview

---

## 🔹 01 — Message Queue Character Driver

### Features

* FIFO queue (16 messages × 128 bytes)
* `read()` / `write()` support
* Multi-process safe
* Kernel-user communication

### Queue Behavior ⚠️

* Fixed-size circular buffer
* When full → **oldest message is overwritten**
* Ensures system always retains **latest data**

This models **streaming systems** where freshness > completeness.

### Concepts Covered

* `file_operations`
* `copy_to_user` / `copy_from_user`
* `cdev` registration
* Synchronization (mutex/spinlock basics)

---

## 🔹 02 — Blocking + Non-Blocking + poll/select

### Features

* Blocking `read()` using wait queues
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

## 🔹 04 — Timer-Based Event Driver

### Features

* Kernel timer (`timer_list`) based event generation
* Periodic event generation (~1 second interval)
* Asynchronous producer (kernel) → consumer (user-space)
* Blocking `read()` using wait queues
* IOCTL control:

  * START_TIMER
  * STOP_TIMER

### Event Format

```
timer_event_<counter>
```

Example:

```
timer_event_1
timer_event_2
```

### Queue Behavior

* FIFO queue (16 × 128 bytes)
* Drop-oldest policy when full
* Models **real-time streaming systems (camera pipelines)**

### Concepts Covered

* Kernel timers (`timer_setup`, `mod_timer`)
* Interrupt context vs process context
* Producer–consumer model
* Wait queue + wakeup integration
* Spinlocks for concurrency protection

---

# 🧪 User-Space Test Programs

---

## 🔸 02_nbread.c

Tests **non-blocking read behavior**

```bash
./02_nbread
```

---

## 🔸 02_polltest.c

Tests **poll/select behavior**

```bash
./02_polltest
```

---

## 🔸 03_ioctl_test.c

Tests IOCTL commands

```bash
./03_ioctl_test
```

---

# ⚙️ Build Instructions

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

# 📟 Device Nodes

```bash
/dev/sanath_queue   # Drivers 01–03
/dev/sanath_timer   # Driver 04
```

---

# 🧪 Example Usage

### Write

```bash
echo "hello" > /dev/sanath_queue
```

### Read

```bash
cat /dev/sanath_queue
```

### Timer Control

```c
ioctl(fd, START_TIMER);
ioctl(fd, STOP_TIMER);
```

---

# 📘 Lessons Learnt

Detailed notes for Timer Driver:

👉 See: `Lessons_Learnt/04_timer_learnings.html`

(Contains insights on timers, wait queues, race conditions, and debugging)

---

# 🧠 What This Repository Demonstrates

* Real Linux driver design patterns
* Blocking vs non-blocking I/O
* Event-driven architecture (`poll/select`)
* IOCTL-based control interfaces
* Asynchronous kernel event generation
* Concurrency handling (spinlocks, wait queues)

---

# 📈 Learning Roadmap

Completed:

* [x] Character driver
* [x] Wait queues
* [x] poll/select
* [x] IOCTL interface
* [x] Kernel timers

Upcoming:

* [ ] Workqueue driver
* [ ] GPIO driver (hardware interaction)
* [ ] Interrupt-driven driver
* [ ] Platform driver (device tree)

---

# ⚠️ Disclaimer

These drivers are implemented for **learning purposes** and are not production-ready.

---

# 👨‍💻 Author

Sanath Kumar P Sapre

---

# ⭐ Final Note

This repository represents a **progressive journey into Linux kernel development**.

Each driver builds on the previous one, moving closer to **real-world embedded systems and device drivers**.