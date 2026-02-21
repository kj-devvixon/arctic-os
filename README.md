# ArcticOS

A bare-metal operating system with a custom kernel, written in C & ASM from scratch.

> Not a Linux distro. Not Unix. Just a custom OS built from the ground up.

---

## Features

- 32-bit Protected Mode (i386/i686)
- GRUB2 Multiboot 1 bootloader
- VESA VBE framebuffer display (800x600+)
- **Custom** desktop manager
- PS/2 keyboard driver (US QWERTY)
- PIT 8253 timer (100Hz)
- CMOS **Real Time Clock**
- IDT + PIC 8259A interrupt handling
- GDT setup
- **Built-in libc** (no stdlib dependency)
- **Custom Graphics Pipeline** – 1-bit monochrome bitmap rendering with byte-aligned row strides for pixel-perfect assets.

### Apps
- **Clock** – analog + digital clock using real RTC data
- **Terminal** – shell with built-in commands
- **Text Editor** – simple editor with basic keybindings

---

## Project Structure

```
arctic-os/
├── Makefile              # Build system
├── linker.ld             # Linker script
├── boot/
│   └── boot.asm          # Bootloader (ASM, Multiboot)
├── kernel/
│   ├── kernel.c          # Kernel entry point
│   ├── gdt.c             # Global Descriptor Table
│   ├── idt.c             # Interrupt Descriptor Table + PIC
|   ├── logo_data.c       # Monochrome logo bitmap data (13-byte stride)
│   └── desktop.c         # Desktop manager
├── drivers/
│   ├── framebuffer.c     # VESA VBE + 8x16 font + graphics
│   ├── keyboard.c        # PS/2 keyboard driver
│   ├── rtc.c             # Real Time Clock (CMOS)
│   └── timer.c           # PIT 8253 (100Hz)
├── apps/
│   ├── clock.c           # Analog + digital clock
│   ├── terminal.c        # Shell
│   └── editor.c          # Text editor
├── libc/
│   └── libc.c            # Custom C library
├── include/
│    ├── logo_data.h      # Headers, types, declarations
|    └── kernel.h         # Headers, types, declarations
└── grub/
    └── grub.cfg          # GRUB config
```

---

## Requirements

```bash
sudo apt install -y \
    nasm gcc gcc-multilib binutils make \
    grub-pc-bin grub-common xorriso mtools \
    qemu-system-x86 qemu-kvm
```

Optional (recommended) – i686 cross-compiler:
```bash
sudo apt install -y gcc-i686-linux-gnu
# Then in Makefile change CROSS_CC to: i686-linux-gnu-gcc
```

---

## Build & Run

```bash
# Build
make

# Create ISO
make iso

# Run in QEMU
make run

# Or manually
qemu-system-i386 -cdrom arcticos.iso -m 128M -vga std -no-reboot
```

---

## Controls

| Key | Action |
|-----|--------|
| `1` | Open Clock |
| `2` | Open Terminal |
| `3` | Open Text Editor |
| `ESC` | Return to desktop |

### Terminal commands
`help`, `time`, `uname`, `cpuid`, `uptime`, `meminfo`, `echo`, `color`, `clear`, `exit`

### Text Editor
`BACKSPACE` delete, `ENTER` new line, `Ctrl+A` line start, `Ctrl+E` line end, `ESC` exit

---

## Run on real hardware

```bash
# WARNING: This will overwrite the USB drive!
sudo dd if=arcticos.iso of=/dev/sdX bs=4M status=progress
```

---

## Technical Details

| Property | Value |
|----------|-------|
| Architecture | i386/i686, 32-bit Protected Mode |
| Boot | GRUB2 Multiboot 1 |
| Display | VESA VBE (800x600+) |
| Font | Built-in 8x16px bitmap (ASCII 32-127) |
| Keyboard | PS/2, US QWERTY |
| Timer | PIT 8253, 100Hz |
| RTC | CMOS 0x70/0x71, BCD + binary |
| Memory | Flat memory model, no MMU/paging |
| Interrupts | IDT, PIC 8259A, IRQ 0,1,8 |

---

## License

[MIT](LICENSE)
