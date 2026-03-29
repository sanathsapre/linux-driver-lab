# 07 — BBB GPIO Driver Learnings

> **Module:** PRD-007 · BBB GPIO Driver
> **Author:** Sanath Sapre
> **Target:** BeagleBone Black (AM335x), kernel 5.10.168-ti-r76
> **Host:** Dell Latitude 5320, Ubuntu 22.04

---

## 01. From Schematic to Driver — The Full Chain

Understanding how a GPIO LED gets from a circuit diagram to a kernel driver call requires tracing through four layers.

---

### Layer 1 — Schematic

The BBB schematic shows USR0 connected like this:

```
VDD_3V3 ──── R (resistor) ──── LED ──── GPIO1_21 (AM335x pin)
```

- The LED anode connects to 3.3V through a current-limiting resistor
- The cathode connects to GPIO1_21
- When GPIO1_21 is driven **HIGH** → voltage across LED → LED **ON**
- When GPIO1_21 is driven **LOW** → no current → LED **OFF**

This is called **active HIGH** configuration — which is why the DTB says `GPIO_ACTIVE_HIGH`.

---

### Layer 2 — SoC (AM335x)

The AM335x has 4 GPIO banks, each with 32 pins:

| Bank | Base Address | Pins |
|------|-------------|------|
| GPIO0 | 0x44E07000 | 0–31 |
| GPIO1 | 0x4804C000 | 32–63 |
| GPIO2 | 0x481AC000 | 64–95 |
| GPIO3 | 0x481AE000 | 96–127 |

Each bank has memory-mapped registers. To drive a pin HIGH, the kernel writes to the `GPIO_DATAOUT` register at the bank's base address + offset.

**Global GPIO number formula:**

```
global_gpio = bank * 32 + pin
```

So GPIO1_21 = 1 × 32 + 21 = **GPIO 53**

The USR LEDs:

| LED  | Bank | Pin | Global GPIO |
|------|------|-----|-------------|
| USR0 | 1    | 21  | 53          |
| USR1 | 1    | 22  | 54          |
| USR2 | 1    | 23  | 55          |
| USR3 | 1    | 24  | 56          |

---

### Layer 3 — Device Tree

The Device Tree describes the hardware to the kernel without hardcoding it in C. For the USR LEDs, `am335x-bone-common.dtsi` contains:

```dts
leds: leds {
    pinctrl-names = "default";
    pinctrl-0 = <&user_leds_s0>;
    compatible = "gpio-leds";

    led2 {
        label = "beaglebone:green:usr0";
        gpios = <&gpio1 21 GPIO_ACTIVE_HIGH>;
        linux,default-trigger = "heartbeat";
        default-state = "off";
    };
    led3 {
        label = "beaglebone:green:usr1";
        gpios = <&gpio1 22 GPIO_ACTIVE_HIGH>;
        linux,default-trigger = "mmc0";
        default-state = "off";
    };
    led4 {
        label = "beaglebone:green:usr2";
        gpios = <&gpio1 23 GPIO_ACTIVE_HIGH>;
        linux,default-trigger = "cpu0";
        default-state = "off";
    };
    led5 {
        label = "beaglebone:green:usr3";
        gpios = <&gpio1 24 GPIO_ACTIVE_HIGH>;
        linux,default-trigger = "mmc1";
        default-state = "off";
    };
};
```

**Key properties explained:**

| Property | Meaning |
|----------|---------|
| `compatible = "gpio-leds"` | Tells kernel to bind the `leds-gpio` driver to this node |
| `gpios = <&gpio1 21 GPIO_ACTIVE_HIGH>` | GPIO controller reference, pin number, polarity |
| `linux,default-trigger = "heartbeat"` | Assign heartbeat trigger after claiming GPIO |
| `label` | Creates `/sys/class/leds/beaglebone:green:usr0/` |
| `leds:` (before leds {) | The phandle label — used by overlays as `&leds` |

---

### Layer 4 — Kernel Driver

Your driver uses the kernel GPIO subsystem APIs — these abstract away the register addresses:

```c
#define GPIO_53 (53)

/* Claim the GPIO */
gpio_request(GPIO_53, "GPIO_53");

/* Configure direction */
gpio_direction_output(GPIO_53, 0);   /* output, initial value LOW */

/* Drive it */
gpio_set_value(GPIO_53, 1);   /* HIGH → LED ON  */
gpio_set_value(GPIO_53, 0);   /* LOW  → LED OFF */

/* Read it */
gpio_state = gpio_get_value(GPIO_53);

/* Export to sysfs */
gpio_export(GPIO_53, false);   /* false = direction cannot be changed from sysfs */

/* Cleanup */
gpio_unexport(GPIO_53);
gpio_free(GPIO_53);
```

Under the hood, `gpio_set_value(53, 1)` resolves bank = 53/32 = 1, pin = 53%32 = 21, then writes to GPIO1's `GPIO_DATAOUT` register at `0x4804C000 + offset`.

**The full chain:**

```
gpio_set_value(53, 1)
  → GPIO subsystem resolves bank 1, pin 21
  → writes to 0x4804C000 + GPIO_DATAOUT
  → electrical signal on GPIO1_21 pin goes HIGH
  → current flows through resistor and LED
  → LED lights up
```

---

## 02. The GPIO Ownership Problem

### What happens at boot

When the kernel boots:

1. U-Boot loads `am335x-boneblack.dtb` and passes it to the kernel
2. Kernel parses the live device tree
3. Sees `compatible = "gpio-leds"` → binds `leds-gpio` driver
4. `leds-gpio` calls `gpio_request(53)`, `gpio_request(54)`, `gpio_request(55)`, `gpio_request(56)`
5. Assigns triggers — heartbeat thread starts toggling GPIO 53 continuously
6. **GPIO 53–56 are now owned by leds-gpio**

### What happens when you insmod your driver

Your driver's `gpio_request(53)` either:
- Fails — GPIO already owned, returns error
- Succeeds with shared access — but heartbeat trigger keeps overwriting your value

Either way, `gpio_set_value()` has no visible effect because the heartbeat thread fights you.

### Runtime workaround

```bash
echo none > /sys/class/leds/beaglebone:green:usr0/trigger
```

This tells the LED subsystem to stop driving the pin (sets trigger to none). It does NOT call `gpio_free()` — leds-gpio still owns the GPIO — but it stops actively toggling it, so your driver's writes take effect.

This must be done **after every boot** before your driver works.

### Permanent fix — disable the DTB node

Edit `am335x-bone-common.dtsi` on your host machine:

```dts
leds: leds {
    status = "disabled";    /* add this line */
    compatible = "gpio-leds";
    /* ... rest unchanged ... */
};
```

`status = "disabled"` tells the kernel to skip driver binding for this node entirely. `leds-gpio` never probes, never calls `gpio_request()`, GPIOs 53–56 are free from boot.

---

## 03. Device Tree — Concepts and Workflow

### What is the Device Tree?

Before Device Tree, board-specific hardware was hardcoded in the kernel source (`arch/arm/mach-*/` C files). Every board change required recompiling the kernel.

Device Tree separated concerns:

| Component | Role | Compiled separately? |
|-----------|------|----------------------|
| Kernel image (zImage) | Drivers — how to drive hardware | Yes |
| DTB | Hardware description — what exists | Yes |
| Bootloader (U-Boot) | Loads both, passes DTB to kernel | — |

This means one kernel binary can run on multiple boards by swapping the DTB.

### File types

| Extension | Description |
|-----------|-------------|
| `.dts` | Device Tree Source — human-readable text |
| `.dtsi` | Device Tree Source Include — shared fragments included by `.dts` |
| `.dtb` | Device Tree Blob — compiled binary, what U-Boot loads |
| `.dtbo` | Device Tree Blob Overlay — compiled overlay, loaded at runtime |

### DTB vs Overlay

| | Base DTB | Overlay (.dtbo) |
|-|----------|-----------------|
| Loaded by | U-Boot at boot | Kernel configfs at runtime |
| Scope | Full board description | Patch to live tree |
| Effective when | Before any driver probes | After boot — too late to prevent already-probed drivers |
| Use case | Board definition, disabling nodes | Cape support, header pin config |

### Why overlays don't work for disabling leds-gpio

The overlay is applied at runtime — **after** `leds-gpio` has already probed and claimed the GPIOs. Setting `status = "disabled"` in an overlay patches the live tree but does not un-probe an already-running driver. The correct fix is to disable the node in the base DTB so `leds-gpio` never probes at all.

Overlays are useful when there is nothing to un-claim — enabling a cape, configuring expansion header pins, adding a new peripheral.

---

## 04. DTB Compilation Workflow

### Cross-compiling the DTB on the host

After editing `am335x-bone-common.dtsi` on your laptop:

```bash
# From kernel source root
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- am335x-boneblack.dtb
```

Output:

```
arch/arm/boot/dts/am335x-boneblack.dtb
```

Copy to BBB:

```bash
# Backup first
ssh debian@<bbb-ip> "cp /boot/dtbs/am335x-boneblack.dtb /boot/dtbs/am335x-boneblack.dtb.bak"

# Deploy
scp arch/arm/boot/dts/am335x-boneblack.dtb debian@<bbb-ip>:/boot/dtbs/am335x-boneblack.dtb

# Reboot
ssh debian@<bbb-ip> "sudo reboot"
```

### Why decompile first (when you don't have source)

If you only have the binary `.dtb` on the target and no source:

```bash
# Decompile binary → text
dtc -I dtb -O dts -o am335x-boneblack.dts /boot/dtbs/am335x-boneblack.dtb

# Edit the .dts
nano am335x-boneblack.dts

# Recompile text → binary
dtc -I dts -O dtb -o am335x-boneblack.dtb am335x-boneblack.dts
```

In your case you have the kernel source on the laptop, so this step is unnecessary — edit the `.dtsi` directly and cross-compile.

### Compiling an overlay

```bash
dtc -O dtb -o am335x-sanath.dtbo -b 0 -@ am335x-sanath.dts
```

| Flag | Meaning |
|------|---------|
| `-O dtb` | Output format: binary |
| `-o file.dtbo` | Output filename |
| `-b 0` | Boot CPU ID (required, set to 0) |
| `-@` | Generate `__symbols__` section — required for phandle resolution at load time |

Without `-@`, the overlay compiles but fails to load because the kernel cannot resolve `&leds` to a node in the live tree.

---

## 05. Key Takeaways

- GPIO number = `bank × 32 + pin` — always compute this from the schematic/SoC manual
- Any GPIO listed in a DTB node under a `compatible` driver is claimed at boot — check DTB before writing a driver that touches those GPIOs
- `leds-gpio` owns USR0–USR3 by default on BBB — disable the node in the base DTB for PRD-08
- Device Tree exists to separate hardware description from kernel code — one kernel, many boards
- Overlays patch the live tree at runtime — too late to prevent already-probed drivers
- Cross-compile DTB with `make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- <target>.dtb`
- Always backup the original DTB before replacing it

---

📁 Path: `linux-driver-lab/07_BBB_GPIO_Driver`
📅 Date: March 2026
