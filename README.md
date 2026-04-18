# linux-driver-lab

A progressive series of Linux kernel character drivers built toward a working understanding of **V4L2 and camera subsystem internals**, developed on a **BeagleBone Black (AM335x)** with cross-compilation from a Dell Latitude 5320.

Each module introduces one new kernel mechanism. The design choices — particularly the drop-oldest queue policy — are deliberately modelled on real streaming pipelines where **data freshness matters more than completeness**.

---

## Target Hardware

| Role | Device |
|---|---|
| Host / cross-compile | Dell Latitude 5320 (x86_64) |
| Target (labs 01–06) | BeagleBone Black (AM335x, ARMv7) |
| Target (labs 07+)   | Raspberry Pi 3 (BCM2837, ARMv7) |

---

## Repository Structure

```
linux-driver-lab/
├── 01_message_queue_driver/     # Character driver with FIFO ring buffer
├── 02_nonblocking_poll_driver/  # Blocking, non-blocking, poll/select
├── 03_ioctl_interface/          # ioctl control interface
├── 04_kernel_timer_driver/      # Kernel timer + async event generation
├── 05_kernel_workqueue_driver/  # Deferred work via workqueues
├── 06_kernel_wq_poll_driver/    # Workqueue + poll/select + non-blocking I/O
├── Makefile
└── README.md
```

Each directory contains the kernel module source, user-space test programs, and a `learnings.md` (drivers 04–06).

---

## Drivers

### 01 — Message Queue Character Driver

A basic character device backed by a fixed-size circular buffer (16 messages × 128 bytes).

**Queue policy:** When the buffer is full, the oldest entry is overwritten. This is a deliberate design choice that mirrors the behaviour of V4L2 streaming buffers — the consumer always gets the most recent data, not stale frames. Blocking on a full queue would be wrong in a streaming context.

**Kernel concepts:** `file_operations`, `cdev` registration, `copy_to_user` / `copy_from_user`, mutex-based concurrency, `class_create` / `device_create` for automatic `/dev` node creation.

---

### 02 — Blocking, Non-Blocking, and poll/select

Extends the message queue driver to support all three I/O modes a real application might use.

**Kernel concepts:** Wait queues, `wake_up_interruptible`, `O_NONBLOCK` handling, `poll_wait`, `POLLIN` / `POLLOUT` mask.

**Test programs:** `02_nbread.c` (non-blocking read), `02_polltest.c` (poll/select).

---

### 03 — ioctl Control Interface

Adds an `ioctl` control plane to the driver, exposing runtime inspection and reset operations.

| Command | Description |
|---|---|
| `GET_QUEUE_SIZE` | Current number of messages in queue |
| `GET_MAX_CAPACITY` | Maximum queue capacity |
| `CLEAR_QUEUE` | Flush all messages |
| `RESET_DEVICE` | Reset driver state |

**Kernel concepts:** `unlocked_ioctl`, `_IOW` / `_IOR` macro definitions, kernel-user control interface design.

**Test program:** `03_ioctl_test.c`.

---

### 04 — Kernel Timer Driver

An asynchronous event producer using `timer_list`. The kernel fires a timer every ~1 second, writes a timestamped event to the ring buffer, and wakes up any blocked reader. Timer control is exposed via ioctl.

**Event format:** `timer_event_<n>`

**ioctl commands:** `START_TIMER`, `STOP_TIMER`

**Device node:** `/dev/sanath_timer`

This driver models the producer side of a camera pipeline — an external source generates frames asynchronously; the consumer reads them via blocking `read()`.

**Kernel concepts:** `timer_setup`, `mod_timer`, `del_timer_sync`, softirq context vs. process context, `spin_lock_irqsave` / `spin_unlock_irqrestore`, `timer_active` flag pattern to prevent re-arm after stop, staging `copy_to_user` outside the spinlock.

**Test programs:** `04_timer_test.c` (start/stop via ioctl), `04_timer_read.c` (blocking read).

---

### 05 — Workqueue Driver

Moves event generation out of the timer callback (softirq / atomic context) and into a kernel workqueue (process context). This is the standard pattern for any non-trivial processing that cannot safely run in atomic context — and is directly analogous to how V4L2 drivers defer frame processing.

**Device node:** `/dev/sanath_worker`

**Kernel concepts:** `DECLARE_WORK`, `schedule_work`, `flush_work`, system workqueue vs. dedicated workqueue, why process context matters (can sleep, can call `kmalloc(GFP_KERNEL)`), bottom-half mechanism selection (workqueue vs. softirq vs. tasklet).

**Test programs:** `05_wq_test.c` (ioctl start/stop), `05_wq_read.c` (blocking read).

---

### 06 — Workqueue + poll/select + Non-Blocking I/O

Combines the workqueue-based async producer from driver 05 with full `poll/select` support and non-blocking read mode. This is the most complete character driver in the series and represents the full I/O model that V4L2 applications rely on — `poll()` / `select()` on `/dev/videoX` alongside `O_NONBLOCK` reads.

**Event format:** `worker_event_<n>`

**Device node:** `/dev/sanath_worker`

**ioctl commands:** `START_TIMER`, `STOP_TIMER`

**Queue policy:** Drop-oldest on full buffer (16 × 128 bytes), identical to the `videobuf2` streaming model.

**Kernel concepts:** `poll_wait`, `POLLIN | POLLRDNORM` mask, `O_NONBLOCK` / `EAGAIN` path, `wait_event_interruptible_exclusive`, combining spinlock-protected state with wait queue wakeup, `file->private_data` for per-fd state, `del_timer_sync` + `flush_work` in module exit for clean teardown.

**Test programs:**

| File | Purpose |
|---|---|
| `06_wq_test.c` | Start/stop timer via ioctl, runs for 30 seconds |
| `06_poll_test.c` | `poll()` with 5-second timeout, then blocking read |
| `06_nbread.c` | Non-blocking read — expects `EAGAIN` on empty buffer |

---

## Build

Cross-compile for BeagleBone Black:

```bash
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
export KDIR=/path/to/bbb-kernel-source

make
```

Native build (for host testing):

```bash
make
```

---

## Deploy and Test

Copy the module to the target:

```bash
scp <driver>.ko user@beaglebone:/tmp/
```

On the target:

```bash
sudo insmod /tmp/<driver>.ko
dmesg | tail -20

# Basic smoke test (drivers 01–03)
echo "hello" > /dev/sanath_queue
cat /dev/sanath_queue

# Workqueue + poll driver (06)
./06_wq_test &        # starts timer via ioctl, runs 30s
./06_poll_test        # poll for first event
./06_nbread           # non-blocking read — EAGAIN if buffer empty

sudo rmmod <driver_name>
dmesg | tail -10
```

---

## Device Nodes

Nodes are created automatically via `class_create` / `device_create` — no manual `mknod` required.

| Node | Used by |
|---|---|
| `/dev/sanath_queue` | Drivers 01–03 |
| `/dev/sanath_timer` | Driver 04 |
| `/dev/sanath_worker` | Drivers 05–06 |

---

## Roadmap

The progression is structured to build the knowledge needed for V4L2 driver development. Each stage maps directly to a concept V4L2 relies on.

| Stage | Driver / Concept | V4L2 Relevance |
|---|---|---|
| ✅ Done | Character driver, ring buffer | Buffer management, `vb2` queue model |
| ✅ Done | Wait queues, poll/select | `select()` on `/dev/videoX` |
| ✅ Done | ioctl interface | `VIDIOC_*` control plane |
| ✅ Done | Kernel timers | Frame timing, periodic capture |
| ✅ Done | Workqueues | Deferred frame processing out of IRQ context |
| ✅ Done | Workqueue + poll + non-blocking | Full async I/O model used by V4L2 applications |
| ✅ Done | GPIO driver (BBB, AM335x) | Sensor control lines, GPIO ownership, Device Tree |
| ✅ Done | Interrupt + Platform driver (RPi3, Yocto) | Hardware capture triggers, SoC peripheral binding |
| 🔲 Next | V4L2 subdev driver (RPi3) | Sensor driver model, format negotiation |
| 🔲 Future | V4L2 driver (videobuf2) | Full camera subsystem integration |
---

## Continuation

The next phase of this series moves to **Raspberry Pi 3** with Yocto/Kirkstone, Device Tree overlays, platform drivers, and V4L2 — documented in [rpi-camera-driver-lab](https://github.com/sanathsapre/rpi-camera-driver-lab).

---

## Disclaimer

These drivers are written for learning purposes and are not production-ready.

---

## Author

Sanath Kumar P Sapre
