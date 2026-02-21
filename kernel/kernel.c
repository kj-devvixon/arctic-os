#include "../include/kernel.h"

framebuffer_t fb;

// Draw a test gradient to verify orientation and colors
static void fb_test_orientation(void) {
    // Corners - no flipping, directly to fb_put_pixel
    // Top-left = RED
    fb_fill_rect(0, 0, 80, 50, 0x00FF0000);
    // Top-right = GREEN
    fb_fill_rect((int)fb.width-80, 0, 80, 50, 0x0000FF00);
    // Bottom-left = BLUE
    fb_fill_rect(0, (int)fb.height-50, 80, 50, 0x000000FF);
    // Bottom-right = WHITE
    fb_fill_rect((int)fb.width-80, (int)fb.height-50, 80, 50, 0x00FFFFFF);

    // Corner labels
    fb_draw_string(2,  2,  "TL=RED",   0x00FFFFFF, 0x00FF0000, 1);
    fb_draw_string((int)fb.width-80, 2,  "TR=GRN", 0x00FFFFFF, 0x0000FF00, 1);
    fb_draw_string(2,  (int)fb.height-16, "BL=BLU", 0x00FFFFFF, 0x000000FF, 1);
    fb_draw_string((int)fb.width-80, (int)fb.height-16, "BR=WHT", 0x00000000, 0x00FFFFFF, 1);

    // Color bar in the center (horizontal gradient R->G->B)
    for (u32 x = 0; x < fb.width; x++) {
        u32 third = fb.width / 3;
        u32 color;
        if (x < third)
            color = 0x00FF0000; // red
        else if (x < 2*third)
            color = 0x0000FF00; // green
        else
            color = 0x000000FF; // blue
        for (int y = (int)fb.height/2 - 10; y < (int)fb.height/2 + 10; y++)
            fb_put_pixel((int)x, y, color);
    }
}

void kernel_main(u32 magic, multiboot_info_t *mbi) {

    // ---- Hardware initialization ----
    gdt_init();
    idt_init();
    pic_init();

    // ---- Framebuffer initialization - detect MB1 vs MB2 ----
    if (magic == MBOOT2_MAGIC) {
        fb_init_mb2((mb2_info_t *)mbi);
    } else {
        // Multiboot 1 (magic == MBOOT1_MAGIC or unknown)
        fb_init(mbi);
    }

    // ---- Orientation test ----
    fb_clear(0x00101828);
    fb_test_orientation();

    char info[128];
    ksprintf(info, "FB: %ux%u %ubpp pitch=%u addr=0x%x",
        fb.width, fb.height, (u32)fb.bpp, fb.pitch, (u32)(u32*)fb.addr);
    fb_draw_string(85, 10, info, 0x00FFFF00, 0x00101828, 1);
    fb_draw_string(85, 28, "Top-left=RED  Top-right=GREEN", 0x00AAAAAA, 0x00101828, 1);
    fb_draw_string(85, 44, "Bottom-left=BLUE  Bottom-right=WHITE", 0x00AAAAAA, 0x00101828, 1);

    // ---- Drivers ----
    timer_init(100);
    keyboard_init();
    rtc_init();

    // ---- Enable interrupts ----
    enable_interrupts();

    // Wait a moment to see the test
    u32 wait = 200; // ~2s at 100Hz
    u32 t0 = 0;
    while (t0 < wait) {
        __asm__ volatile("hlt");
        t0++;
    }

    // ---- Launch desktop ----
    desktop_init();
    desktop_run();

    for (;;) { __asm__ volatile("hlt"); }
}
