#include "../include/kernel.h"

// ============================================================
// DESKTOP CONSTANTS
// ============================================================
#define TASKBAR_H     40
#define ICON_SIZE     64
#define ICON_PADDING  20

// ============================================================
// APPLICATION ICONS
// ============================================================
typedef struct {
    const char *name;
    const char *icon_label;
    u32  color;
    void (*run)(void);
    int  x, y;
} desktop_icon_t;

static desktop_icon_t icons[] = {
    { "Clock",    "RTC",  0x001A6AAF, app_clock_run,    0, 0 },
    { "Terminal", "TERM", 0x001A4A1A, app_terminal_run, 0, 0 },
    { "Editor",   "EDIT", 0x004A2A0A, app_editor_run,   0, 0 },
};
#define ICON_COUNT 3

static int selected_icon = -1;

// ============================================================
// DRAW BACKGROUND (arctic gradient)
// ============================================================
static void draw_background(void) {
    u32 w = fb.width;
    u32 h = fb.height - TASKBAR_H;
    for (u32 y = 0; y < h; y++) {
        // Gradient from dark blue to navy
        u32 ratio = y * 255 / h;
        u8 r = 0x00 + (ratio * 0x05 / 255);
        u8 g = 0x08 + (ratio * 0x18 / 255);
        u8 b = 0x18 + (ratio * 0x38 / 255);
        u32 color = (r << 16) | (g << 8) | b;
        for (u32 x = 0; x < w; x++)
            fb_put_pixel(x, y, color);
    }

    // Snowflakes / aurora effect
    // Random brightness points (seeded)
    for (int i = 0; i < 200; i++) {
        int sx = (i * 1337 + i * i * 47) % w;
        int sy = (i * 2341 + i * 113) % (int)h;
        u8 bright = 0x40 + (i * 73) % 0x80;
        u32 star = (bright << 16) | (bright << 8) | 0xFF;
        fb_put_pixel(sx, sy, star);
        if (i % 7 == 0) {
            fb_put_pixel(sx+1, sy, star);
            fb_put_pixel(sx, sy+1, star);
        }
    }

    // Aurora (green-blue bands)
    for (u32 x = 0; x < w; x++) {
        int aurora_y = 50 + (int)(30.0f * ((x % 200) < 100 ?
            (float)(x % 200) / 100.0f :
            (float)(200 - x % 200) / 100.0f));
        for (int t = 0; t < 8; t++) {
            u8 alpha = 0x20 - t * 3;
            u32 ac = ((u32)(alpha/4) << 16) | ((u32)(alpha) << 8) | (alpha/2);
            fb_put_pixel(x, aurora_y + t, ac);
        }
    }
}

// ============================================================
// DRAW TASKBAR
// ============================================================
static void draw_taskbar(void) {
    int bar_y = fb.height - TASKBAR_H;

    // Taskbar background - dark blue with shine
    fb_fill_rect(0, bar_y, fb.width, TASKBAR_H, COLOR_ARCTIC_BAR);
    fb_fill_rect(0, bar_y, fb.width, 1, 0x002255AA); // top border

    // ArcticOS logo
    fb_fill_rect(5, bar_y + 5, 80, 30, COLOR_ARCTIC_BTN);
    fb_draw_rect(5, bar_y + 5, 80, 30, COLOR_ARCTIC_ACC, 1);
    fb_draw_string(13, bar_y + 12, "ArcticOS", COLOR_ARCTIC_ACC, COLOR_ARCTIC_BTN, 1);

    // Clock on taskbar (RTC)
    rtc_time_t t;
    rtc_read(&t);
    t.hour = (t.hour + 1) % 24; // UTC+1 (CET)
    char time_str[32];
    ksprintf(time_str, "%02d:%02d:%02d", (u32)t.hour, (u32)t.minute, (u32)t.second);
    char date_str[32];
    ksprintf(date_str, "%s %02d.%02d.%u",
        rtc_weekday_str(t.weekday), (u32)t.day, (u32)t.month, (u32)t.year);

    int time_x = fb.width - 130;
    fb_fill_rect(time_x - 5, bar_y + 3, 125, 34, COLOR_ARCTIC_WIN);
    fb_draw_rect(time_x - 5, bar_y + 3, 125, 34, 0x002255AA, 1);
    fb_draw_string(time_x, bar_y + 6,  time_str, COLOR_ARCTIC_ACC, COLOR_ARCTIC_WIN, 1);
    fb_draw_string(time_x, bar_y + 22, date_str, COLOR_LIGHT_GRAY, COLOR_ARCTIC_WIN, 1);

    // Active app description
    fb_draw_string(100, bar_y + 13, "Click an app icon above [1][2][3]",
        COLOR_LIGHT_GRAY, COLOR_ARCTIC_BAR, 1);
}

// ============================================================
// DRAW APPLICATION ICONS
// ============================================================
static void draw_icons(void) {
    int start_x = 30;
    int start_y = 30;

    for (int i = 0; i < ICON_COUNT; i++) {
        int ix = start_x + i * (ICON_SIZE + ICON_PADDING);
        int iy = start_y;
        icons[i].x = ix;
        icons[i].y = iy;

        u32 bg = icons[i].color;
        bool sel = (selected_icon == i);

        // Shadow
        fb_fill_rect(ix+4, iy+4, ICON_SIZE, ICON_SIZE, 0x00080808);

        // Icon background
        fb_fill_rect(ix, iy, ICON_SIZE, ICON_SIZE, bg);

        // Border
        fb_draw_rect(ix, iy, ICON_SIZE, ICON_SIZE,
            sel ? COLOR_ARCTIC_ACC : 0x00336699, sel ? 2 : 1);

        // Center label (large)
        fb_draw_string(ix + 8, iy + 18, icons[i].icon_label,
            COLOR_WHITE, bg, 2);

        // Shortcut number
        char num[3] = {'[', '1'+i, ']'};
        num[2] = 0;
        fb_draw_string(ix + 2, iy + 2, num, COLOR_ARCTIC_ACC, bg, 1);

        // Name below icon
        int name_len = kstrlen(icons[i].name) * 8;
        int name_x = ix + (ICON_SIZE - name_len) / 2;
        fb_fill_rect(ix - 2, iy + ICON_SIZE + 2, ICON_SIZE + 4, 16, COLOR_ARCTIC_BG);
        fb_draw_string(name_x, iy + ICON_SIZE + 3, icons[i].name,
            COLOR_TEXT_BRIGHT, COLOR_ARCTIC_BG, 1);
    }
}

// ============================================================
// DRAW FULL DESKTOP
// ============================================================
void desktop_draw(void) {
    draw_background();
    draw_icons();
    draw_taskbar();
}

// ============================================================
// INITIALIZATION
// ============================================================
void desktop_init(void) {
    fb_clear(COLOR_ARCTIC_BG);
    desktop_draw();
}

// ============================================================
// MAIN DESKTOP LOOP
// ============================================================
void desktop_run(void) {
    u32 last_ticks = 0;

    while (1) {
        // Refresh every ~1s (100 ticks at 100Hz)
        u32 t = timer_get_ticks();
        if (t - last_ticks >= 100) {
            last_ticks = t;
            draw_taskbar(); // Refresh clock
        }

        // Keyboard input
        if (keyboard_has_char()) {
            char c = keyboard_getchar();
            int app = -1;
            if (c == '1') app = 0;
            else if (c == '2') app = 1;
            else if (c == '3') app = 2;

            if (app >= 0 && app < ICON_COUNT) {
                selected_icon = app;
                desktop_draw();
                // Short visual pause
                u32 wait = timer_get_ticks() + 20;
                while (timer_get_ticks() < wait) __asm__("hlt");
                // Launch app
                icons[app].run();
                // After return: refresh desktop
                selected_icon = -1;
                desktop_draw();
            }
        }

        __asm__ volatile("hlt");
    }
}
