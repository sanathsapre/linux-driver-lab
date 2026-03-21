# 🚀 04 — Kernel Timer Driver Learnings

> **Module:** PRD-004 · Kernel Timer Driver  
> **Author:** Sanath Sapre  
> **Kernel:** 6.8.0-106-generic  
> **Host:** Dell Latitude 5320  
> **Version:** 1.4  

---

## 📌 01. Driver Overview

| Parameter        | Value |
|-----------------|------|
| Device Node     | `/dev/sanath_timer` |
| Buffer          | 16 rows × 128 bytes |
| Timer Interval  | 1000 ms |
| Synchronization | `spinlock_t (irqsave/irqrestore)` |
| IOCTL Magic     | `'f'` |
| IOCTL Commands  | `START_TIMER`, `STOP_TIMER` |
| File Ops        | `open`, `release`, `read`, `unlocked_ioctl` |

A periodic kernel timer fires every 1000 ms and writes:

```
timer_event_N\n
```

into a ring buffer. Userspace reads via device node. Timer control is via ioctl.

---

## ⚙️ 02. Spinlock vs Mutex — Context Matters

Timer callback runs in **softirq context**.

| Primitive     | Can Sleep | Safe in SoftIRQ |
|--------------|----------|-----------------|
| mutex        | ❌ YES   | ❌ NO |
| spinlock_t   | ✅ NO    | ✅ YES |

### 🔍 spin_lock_irqsave Explained

```c
unsigned long flags;

spin_lock_irqsave(&lock, flags);
/* critical section */
spin_unlock_irqrestore(&lock, flags);
```

- Saves interrupt state before locking  
- Restores exact state after unlocking  
- Macro (not function)

### 🧠 Golden Rule
Keep critical section minimal:
- Only shared data inside  
- Move everything else outside:
  - `wake_up`
  - `mod_timer`
  - `copy_to_user`

---

## 🚫 03. What Cannot Be Called Under Spinlock

| Function | Reason | Safe? |
|----------|--------|------|
| wake_up_interruptible | touches scheduler | ❌ NO |
| copy_to_user | may page fault | ❌ NEVER |
| kmalloc(GFP_KERNEL) | may sleep | ❌ NEVER |
| mod_timer | has internal locks | ⚠️ RISKY |
| memcpy / snprintf | safe | ✅ YES |

### ✅ Correct Pattern

```c
spin_lock_irqsave(&lock, flags);

snprintf(buffer[idx], MEM_SIZE, "timer_event_%ld\n", count++);
idx = (idx + 1) % ROW_SIZE;

is_active = timer_active;

spin_unlock_irqrestore(&lock, flags);

wake_up_interruptible(&queue);

if (is_active)
    mod_timer(&timer, jiffies + msecs_to_jiffies(TIMEOUT));
```

---

## ⚠️ 04. copy_to_user Must Not Be Under Spinlock

### ❌ Problem
- `copy_to_user` may sleep → illegal in spinlock  
- Causes `EFAULT`

### ✅ Fix

```c
uint8_t tmp[MEM_SIZE];

spin_lock_irqsave(&lock, flags);

memcpy(tmp, buffer[idx], len);
memset(buffer[idx], 0, MEM_SIZE);

spin_unlock_irqrestore(&lock, flags);

if (copy_to_user(user_buf, tmp, len))
    return -EFAULT;
```

---

## 🔄 05. Timer Active Flag

### ❌ Problem
`del_timer_sync()` does not stop already running callback → timer re-arms

### ✅ Fix

```c
/* struct */
int timer_active;

/* START */
timer_active = 1;
mod_timer(...);

/* STOP */
timer_active = 0;
del_timer_sync(...);

/* callback */
if (is_active)
    mod_timer(...);
```

---

## ⚡ 06. del_timer vs del_timer_sync

| Function | Behaviour | Use |
|---------|----------|-----|
| del_timer | does NOT wait for callback | ❌ AVOID |
| del_timer_sync | waits for callback | ✅ ALWAYS |

Using `del_timer` → possible kernel crash

---

## 📏 07. Return Actual String Length

### ❌ Problem
Returning `MEM_SIZE` causes:
- garbage bytes  
- corrupted output  

### ✅ Fix

```c
msg_len  = strnlen(buffer[idx], MEM_SIZE);
copy_len = min(len, msg_len);

memcpy(tmp, buffer[idx], copy_len);

return copy_len;
```

### 🚨 Critical Rule
Compute length **AFTER wait and inside spinlock**

---

## 🐞 08. Bugs Encountered & Fixed

| Bug | Fix |
|-----|----|
| write index not incremented | circular increment |
| timer not stopping | timer_active flag |
| del_timer used | replaced with del_timer_sync |
| copy_to_user under spinlock | moved outside |
| wake_up under spinlock | moved outside |
| msg_len computed early | compute after wait |
| returning MEM_SIZE | return actual length |

---

## 🧪 09. Test Sequence

```bash
make
sudo insmod 04_timer_driver.ko
./04_timer_test
./04_timer_read
dmesg | grep timer_event
sudo rmmod 04_timer_driver
```

---

## 🎯 Final Insight

This module demonstrates:

- Softirq-safe synchronization  
- Timer lifecycle handling  
- Safe user-space interaction  
- Real-world debugging patterns  

---

📁 Path: `linux-driver-lab/04_kernel_timer_driver`  
📅 Date: March 2026
