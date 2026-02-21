#include "../include/kernel.h"

// Simple trig functions (sin/cos via lookup table)
// 360 degrees, values scaled * 1000
static const int sin_table[91] = {
    0,17,35,52,70,87,105,122,139,156,174,191,208,225,242,
    259,276,292,309,326,342,358,375,391,407,423,438,454,469,485,
    500,515,530,545,559,574,588,602,616,629,643,656,669,682,695,
    707,719,731,743,755,766,777,788,799,809,819,829,839,848,857,
    866,875,883,891,899,906,914,921,927,934,940,946,951,956,961,
    966,970,974,978,982,985,988,990,993,995,996,998,999,999,1000
};

static int isin(int deg) {
    deg = ((deg % 360) + 360) % 360;
    if (deg <= 90) return sin_table[deg];
    if (deg <= 180) return sin_table[180 - deg];
    if (deg <= 270) return -sin_table[deg - 180];
    return -sin_table[360 - deg];
}

static int icos(int deg) {
    return isin(deg + 90);
}

// Draw a clock hand
static void draw_hand(int cx, int cy, int angle_deg, int len, int thick, u32 color) {
    int ex = cx + (icos(angle_deg - 90) * len) / 1000;
    int ey = cy + (isin(angle_deg - 90) * len) / 1000;
    for (int t = -thick; t <= thick; t++) {
        fb_draw_line(cx + t, cy, ex + t, ey, color);
        fb_draw_line(cx, cy + t, ex, ey + t, color);
    }
}

// Draw clock face
static void draw_clock_face(int cx, int cy, int r) {
    // Circular background
    fb_fill_circle(cx, cy, r + 5, 0x00050F20);
    fb_fill_circle(cx, cy, r, COLOR_ARCTIC_WIN);

    // Outer ring
    for (int i = r-3; i <= r+3; i++)
        fb_fill_circle(cx, cy, i, i == r ? COLOR_ARCTIC_ACC : 0);

    // Hour markers
    for (int h = 0; h < 12; h++) {
        int angle = h * 30 - 90;
        int x1 = cx + (icos(angle) * (r - 5)) / 1000;
        int y1 = cy + (isin(angle) * (r - 5)) / 1000;
        int x2 = cx + (icos(angle) * (r - 15)) / 1000;
        int y2 = cy + (isin(angle) * (r - 15)) / 1000;
        fb_draw_line(x1, y1, x2, y2, h % 3 == 0 ? COLOR_ARCTIC_ACC : COLOR_LIGHT_GRAY);
        if (h % 3 == 0) {
            // Numbers: 12, 3, 6, 9
            char num_s[3];
            int hour_num = h == 0 ? 12 : h;
            kitoa(hour_num, num_s, 10);
            int tx = cx + (icos(angle) * (r - 28)) / 1000;
            int ty = cy + (isin(angle) * (r - 28)) / 1000;
            fb_draw_string(tx - (kstrlen(num_s)*4), ty - 8, num_s, COLOR_TEXT_BRIGHT, COLOR_ARCTIC_WIN, 1);
        }
    }

    // Center dot
    fb_fill_circle(cx, cy, 5, COLOR_ARCTIC_ACC);
}

void app_clock_run(void) {
    // === App window ===
    int win_x = 60, win_y = 30;
    int win_w = fb.width - 120;
    int win_h = fb.height - 100;

    // Window background
    fb_fill_rect(win_x, win_y, win_w, win_h, COLOR_ARCTIC_WIN);
    fb_draw_rect(win_x, win_y, win_w, win_h, COLOR_ARCTIC_ACC, 2);

    // Title bar
    fb_fill_rect(win_x, win_y, win_w, 28, COLOR_ARCTIC_BAR);
    fb_draw_string(win_x + 10, win_y + 7, "Clock / RTC", COLOR_ARCTIC_ACC, COLOR_ARCTIC_BAR, 1);
    fb_draw_string(win_x + win_w - 64, win_y + 7, "[ESC]=Exit", COLOR_LIGHT_GRAY, COLOR_ARCTIC_BAR, 1);

    int cx = win_x + win_w/2;
    int cy = win_y + 40 + 110;
    int clock_r = 100;

    char prev_time[32] = "";

    while (1) {
        rtc_time_t t;
        rtc_read(&t);
        t.hour = (t.hour + 1) % 24; // UTC+1 (CET)

        char time_str[32];
        ksprintf(time_str, "%02d:%02d:%02d", (u32)t.hour, (u32)t.minute, (u32)t.second);

        // Redraw only when time has changed
        if (kstrcmp(time_str, prev_time) != 0) {
            kstrcpy(prev_time, time_str);

            // Clock face
            draw_clock_face(cx, cy, clock_r);

            // Hands
            int sec_angle   = t.second * 6;
            int min_angle   = t.minute * 6 + t.second / 10;
            int hour_angle  = (t.hour % 12) * 30 + t.minute / 2;

            draw_hand(cx, cy, hour_angle,  60, 3, COLOR_TEXT_BRIGHT);
            draw_hand(cx, cy, min_angle,   85, 2, COLOR_WHITE);
            draw_hand(cx, cy, sec_angle,   90, 1, 0x00FF4444);

            // Center dot
            fb_fill_circle(cx, cy, 4, COLOR_ARCTIC_ACC);

            // Digital clock
            int dbox_x = win_x + win_w/2 - 80;
            int dbox_y = win_y + 40 + 230;
            fb_fill_rect(dbox_x, dbox_y, 160, 36, 0x000A1A30);
            fb_draw_rect(dbox_x, dbox_y, 160, 36, COLOR_ARCTIC_ACC, 1);
            fb_draw_string(dbox_x + 16, dbox_y + 9, time_str, COLOR_ARCTIC_ACC, 0x000A1A30, 2);

            // Date
            char date_str[64];
            ksprintf(date_str, "%s, %02d %s %u",
                rtc_weekday_str(t.weekday), (u32)t.day,
                rtc_month_str(t.month), (u32)t.year);
            int date_x = win_x + win_w/2 - kstrlen(date_str)*4;
            fb_fill_rect(win_x + 20, dbox_y + 42, win_w - 40, 16, COLOR_ARCTIC_WIN);
            fb_draw_string(date_x, dbox_y + 42, date_str, COLOR_LIGHT_GRAY, COLOR_ARCTIC_WIN, 1);

            // Timezone info
            fb_draw_string(win_x + 10, win_y + win_h - 25,
                "Time from CMOS RTC | ArcticOS v1.0", COLOR_LIGHT_GRAY, COLOR_ARCTIC_WIN, 1);
        }

        // Keyboard input
        if (keyboard_has_char()) {
            char c = keyboard_getchar();
            if (c == 27 || c == 'q' || c == 'Q') break;
        }

        // Wait ~100ms
        u32 wait_end = timer_get_ticks() + 10;
        while (timer_get_ticks() < wait_end) __asm__("hlt");
    }
}
