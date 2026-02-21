#include "../include/kernel.h"

typedef struct {
    u16 offset_low;
    u16 selector;
    u8  zero;
    u8  type_attr;
    u16 offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t   idt_ptr_s;

// ASM stub declarations
extern void isr0_asm(void);  extern void isr1_asm(void);
extern void isr2_asm(void);  extern void isr3_asm(void);
extern void isr4_asm(void);  extern void isr5_asm(void);
extern void isr6_asm(void);  extern void isr7_asm(void);
extern void isr8_asm(void);  extern void isr13_asm(void);
extern void isr14_asm(void);
extern void irq0_handler_asm(void);
extern void irq1_handler_asm(void);
extern void irq8_handler_asm(void);

static void idt_set(int idx, u32 offset, u16 sel, u8 flags) {
    idt[idx].offset_low  = offset & 0xFFFF;
    idt[idx].offset_high = (offset >> 16) & 0xFFFF;
    idt[idx].selector    = sel;
    idt[idx].zero        = 0;
    idt[idx].type_attr   = flags;
}

void idt_init(void) {
    idt_ptr_s.limit = sizeof(idt) - 1;
    idt_ptr_s.base  = (u32)&idt;

    kmemset(idt, 0, sizeof(idt));

    // CPU exceptions
    idt_set(0,  (u32)isr0_asm,  0x08, 0x8E);
    idt_set(1,  (u32)isr1_asm,  0x08, 0x8E);
    idt_set(2,  (u32)isr2_asm,  0x08, 0x8E);
    idt_set(3,  (u32)isr3_asm,  0x08, 0x8E);
    idt_set(4,  (u32)isr4_asm,  0x08, 0x8E);
    idt_set(5,  (u32)isr5_asm,  0x08, 0x8E);
    idt_set(6,  (u32)isr6_asm,  0x08, 0x8E);
    idt_set(7,  (u32)isr7_asm,  0x08, 0x8E);
    idt_set(8,  (u32)isr8_asm,  0x08, 0x8E);
    idt_set(13, (u32)isr13_asm, 0x08, 0x8E);
    idt_set(14, (u32)isr14_asm, 0x08, 0x8E);

    // IRQs (after PIC remap: IRQ0=0x20, IRQ1=0x21, IRQ8=0x28)
    idt_set(0x20, (u32)irq0_handler_asm, 0x08, 0x8E);
    idt_set(0x21, (u32)irq1_handler_asm, 0x08, 0x8E);
    idt_set(0x28, (u32)irq8_handler_asm, 0x08, 0x8E);

    idt_load(&idt_ptr_s);
}

// ---- PIC ----
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

void pic_init(void) {
    // ICW1
    outb(PIC1_CMD,  0x11);
    outb(PIC2_CMD,  0x11);
    // ICW2: remap IRQ 0-15 to 0x20-0x2F
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    // ICW3
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    // ICW4
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    // Mask: enable IRQ0, IRQ1, IRQ8
    outb(PIC1_DATA, 0b11111100);
    outb(PIC2_DATA, 0b11111110);
}

static const char *exception_names[] = {
    "Division By Zero", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bound Range Exceeded", "Invalid Opcode",
    "Device Not Available", "Double Fault", "Coprocessor Segment",
    "Invalid TSS", "Segment Not Present", "Stack-Segment Fault",
    "General Protection Fault", "Page Fault"
};

void isr_handler(int num) {
    // Display BSOD / panic screen
    fb_fill_rect(0, 0, fb.width, fb.height, 0x000000CC);
    fb_draw_string(10, 10, "=== ArcticOS KERNEL PANIC ===", COLOR_WHITE, 0x000000CC, 2);
    char buf[64];
    ksprintf(buf, "Exception: %d", num);
    fb_draw_string(10, 50, buf, COLOR_YELLOW, 0x000000CC, 2);
    if (num < 15) {
        fb_draw_string(10, 90, exception_names[num], COLOR_WHITE, 0x000000CC, 2);
    }
    fb_draw_string(10, 140, "System halted. Restart required.", COLOR_LIGHT_GRAY, 0x000000CC, 1);
    disable_interrupts();
    for (;;) { __asm__ volatile("hlt"); }
}

void irq_handler(int irq_num) {
    switch (irq_num) {
        case 0: timer_sleep(0); break;  // Timer tick (handled internally)
        case 1: keyboard_handler(); break;
        case 8: rtc_handler(); break;
    }
    // EOI
    if (irq_num >= 8)
        outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}
