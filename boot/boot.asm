; ArcticOS Bootloader
; Supports BOTH Multiboot 1 (BIOS/QEMU) AND Multiboot 2 (UEFI)
; Author: ArcticOS Project

; ============================================================
; MULTIBOOT 1 - for BIOS / legacy GRUB / QEMU
; ============================================================
MBOOT1_MAGIC      equ 0x1BADB002
MBOOT1_FLAGS      equ (1<<0) | (1<<1) | (1<<2)
MBOOT1_CHECKSUM   equ -(MBOOT1_MAGIC + MBOOT1_FLAGS)

; ============================================================
; MULTIBOOT 2 - for UEFI / modern hardware
; ============================================================
MBOOT2_MAGIC      equ 0xE85250D6
MBOOT2_ARCH       equ 0
MBOOT2_LENGTH     equ (mboot2_end - mboot2_start)
MBOOT2_CHECKSUM   equ -(MBOOT2_MAGIC + MBOOT2_ARCH + MBOOT2_LENGTH)

section .multiboot
align 4
; --- Multiboot 1 header ---
    dd MBOOT1_MAGIC
    dd MBOOT1_FLAGS
    dd MBOOT1_CHECKSUM
    dd 0, 0, 0, 0, 0
    dd 0
    dd 800
    dd 600
    dd 32

; --- Multiboot 2 header ---
align 8
mboot2_start:
    dd MBOOT2_MAGIC
    dd MBOOT2_ARCH
    dd MBOOT2_LENGTH
    dd MBOOT2_CHECKSUM

    ; Tag: framebuffer request
    align 8
    dw 5            ; type = framebuffer
    dw 1            ; flags = optional
    dd 20           ; size
    dd 800          ; width
    dd 600          ; height
    dd 32           ; depth

    ; Tag: end
    align 8
    dw 0
    dw 0
    dd 8
mboot2_end:

section .bss
align 16
stack_bottom:
    resb 65536
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    push ebx    ; MB info pointer
    push eax    ; magic (0x2BADB002 = MB1, 0x36D76289 = MB2)
    cld
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang

global outb
outb:
    mov dx, [esp+4]
    mov al, [esp+8]
    out dx, al
    ret

global inb
inb:
    mov dx, [esp+4]
    xor eax, eax
    in al, dx
    ret

global outw
outw:
    mov dx, [esp+4]
    mov ax, [esp+8]
    out dx, ax
    ret

global inw
inw:
    mov dx, [esp+4]
    xor eax, eax
    in ax, dx
    ret

global enable_interrupts
enable_interrupts:
    sti
    ret

global disable_interrupts
disable_interrupts:
    cli
    ret

global idt_load
idt_load:
    mov eax, [esp+4]
    lidt [eax]
    ret

global gdt_load
gdt_load:
    mov eax, [esp+4]
    lgdt [eax]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
.flush:
    ret

%macro IRQ_STUB 1
global irq%1_handler_asm
irq%1_handler_asm:
    pusha
    push dword %1
    extern irq_handler
    call irq_handler
    add esp, 4
    popa
    iret
%endmacro

IRQ_STUB 0
IRQ_STUB 1
IRQ_STUB 8

%macro ISR_STUB 1
global isr%1_asm
isr%1_asm:
    pusha
    push dword %1
    extern isr_handler
    call isr_handler
    add esp, 4
    popa
    iret
%endmacro

ISR_STUB 0
ISR_STUB 1
ISR_STUB 2
ISR_STUB 3
ISR_STUB 4
ISR_STUB 5
ISR_STUB 6
ISR_STUB 7
ISR_STUB 8
ISR_STUB 13
ISR_STUB 14

global rdtsc_low
rdtsc_low:
    rdtsc
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
