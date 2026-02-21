# ============================================================
# ArcticOS Makefile
# Compiler: i686-elf-gcc (cross-compiler) or gcc with flags
# Assembler: nasm
# ============================================================

# Check if we have a cross-compiler
CROSS_CC := i686-elf-gcc
SYSTEM_CC := gcc

# Auto-detect available compiler
ifeq ($(shell which $(CROSS_CC) 2>/dev/null),)
    CC := $(SYSTEM_CC)
    LD := ld
    CROSS := 0
else
    CC := $(CROSS_CC)
    LD := i686-elf-ld
    CROSS := 1
endif

AS := nasm
OBJCOPY := objcopy

# ============================================================
# COMPILATION FLAGS
# ============================================================
CFLAGS := -std=c99 \
           -ffreestanding \
           -fno-stack-protector \
           -fno-builtin \
           -m32 \
           -O2 \
           -Wall \
           -Wextra \
           -Wno-unused-parameter \
           -I./include

# If using system gcc (not cross-compiler)
ifeq ($(CROSS),0)
    CFLAGS += -mno-red-zone \
              -mno-mmx \
              -mno-sse \
              -mno-sse2 \
              -fno-pic \
              -nostdlib \
              -nostdinc \
              -isystem /usr/lib/gcc/x86_64-linux-gnu/*/include \
              -D__KERNEL__
endif

ASFLAGS := -f elf32

LDFLAGS := -m elf_i386 \
            -T linker.ld \
            --oformat=elf32-i386

# ============================================================
# SOURCE FILES
# ============================================================
ASM_SOURCES := boot/boot.asm

C_SOURCES := kernel/kernel.c \
             kernel/gdt.c \
             kernel/idt.c \
             kernel/desktop.c \
             drivers/framebuffer.c \
             drivers/keyboard.c \
             drivers/rtc.c \
             drivers/timer.c \
             apps/clock.c \
             apps/terminal.c \
             apps/editor.c \
             libc/libc.c

# ============================================================
# OBJECTS
# ============================================================
BUILD_DIR := build

ASM_OBJECTS := $(patsubst %.asm, $(BUILD_DIR)/%.o, $(ASM_SOURCES))
C_OBJECTS   := $(patsubst %.c,   $(BUILD_DIR)/%.o, $(C_SOURCES))
ALL_OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

# ============================================================
# TARGETS
# ============================================================
.PHONY: all clean iso run run-nographic help

all: $(BUILD_DIR)/arcticos.elf

# Link kernel
$(BUILD_DIR)/arcticos.elf: $(ALL_OBJECTS) linker.ld
	@echo "[LD] Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS)
	@echo "[OK] Kernel: $@"
	@size $@

# Compile ASM
$(BUILD_DIR)/boot/%.o: boot/%.asm
	@mkdir -p $(dir $@)
	@echo "[AS] $<"
	$(AS) $(ASFLAGS) $< -o $@

# Compile C
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================
# ISO (for running in QEMU)
# ============================================================
iso: $(BUILD_DIR)/arcticos.elf
	@echo "[ISO] Creating ISO image..."
	@mkdir -p isodir/boot/grub
	@cp $(BUILD_DIR)/arcticos.elf isodir/boot/
	@cp grub/grub.cfg isodir/boot/grub/
	grub-mkrescue -o arcticos.iso isodir
	@echo "[OK] Image: arcticos.iso"
	@ls -lh arcticos.iso

# ============================================================
# RUN IN QEMU
# ============================================================
run: iso
	@echo "[QEMU] Starting ArcticOS..."
	qemu-system-i386 \
		-cdrom arcticos.iso \
		-m 128M \
		-vga cirrus \
		-no-reboot \
		-no-shutdown

run-vmware: iso
	qemu-system-i386 -cdrom arcticos.iso -m 128M -vga vmware -no-reboot -no-shutdown

run-std: iso
	qemu-system-i386 -cdrom arcticos.iso -m 128M -vga std -no-reboot -no-shutdown

run-kernel: $(BUILD_DIR)/arcticos.elf
	@echo "[QEMU] Running kernel directly..."
	qemu-system-i386 \
		-kernel $(BUILD_DIR)/arcticos.elf \
		-m 128M \
		-vga std \
		-serial stdio \
		-no-reboot

run-nographic: iso
	qemu-system-i386 \
		-cdrom arcticos.iso \
		-m 128M \
		-nographic \
		-no-reboot

# ============================================================
# DEBUG (with GDB)
# ============================================================
debug: iso
	qemu-system-i386 \
		-cdrom arcticos.iso \
		-m 128M \
		-vga std \
		-s -S \
		-no-reboot &
	sleep 1
	gdb $(BUILD_DIR)/arcticos.elf \
		-ex "target remote localhost:1234" \
		-ex "break kernel_main" \
		-ex "continue"

# ============================================================
# CLEAN
# ============================================================
clean:
	@echo "[CLN] Cleaning..."
	rm -rf $(BUILD_DIR) isodir arcticos.iso
	@echo "[OK] Done"

help:
	@echo "=== ArcticOS Makefile ==="
	@echo "  make         - build kernel"
	@echo "  make iso     - create ISO image"
	@echo "  make run     - build and run in QEMU"
	@echo "  make debug   - run with GDB debugger"
	@echo "  make clean   - clean build files"
