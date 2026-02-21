#include "../include/kernel.h"

typedef struct {
    u16 limit_low;
    u16 base_low;
    u8  base_mid;
    u8  access;
    u8  granularity;
    u8  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__((packed)) gdt_ptr_t;

static gdt_entry_t gdt[5];
static gdt_ptr_t   gdt_ptr;

static void gdt_set(int idx, u32 base, u32 limit, u8 access, u8 gran) {
    gdt[idx].base_low    = base & 0xFFFF;
    gdt[idx].base_mid    = (base >> 16) & 0xFF;
    gdt[idx].base_high   = (base >> 24) & 0xFF;
    gdt[idx].limit_low   = limit & 0xFFFF;
    gdt[idx].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[idx].access      = access;
}

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (u32)&gdt;

    gdt_set(0, 0, 0,          0x00, 0x00); // Null
    gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code
    gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data
    gdt_set(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code
    gdt_set(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data

    gdt_load(&gdt_ptr);
}
