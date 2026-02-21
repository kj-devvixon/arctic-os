#include "../include/kernel.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_FREQ 1193180

static volatile u32 ticks = 0;
static u32 ticks_per_ms = 0;

// Called by irq_handler when IRQ0 fires
static void pit_tick(void) {
    ticks++;
}

void timer_init(u32 freq) {
    u32 divisor = PIT_BASE_FREQ / freq;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
    ticks_per_ms = freq / 1000;
    if (ticks_per_ms == 0) ticks_per_ms = 1;
}

u32 timer_get_ticks(void) {
    return ticks;
}

// Called by IRQ0 handler
void timer_tick_internal(void) {
    pit_tick();
}

void timer_sleep(u32 ms) {
    if (ms == 0) {
        // Called from irq_handler - just increment ticks
        ticks++;
        return;
    }
    u32 target = ticks + ms * ticks_per_ms;
    while (ticks < target) {
        __asm__ volatile("hlt");
    }
}
