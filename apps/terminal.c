#include "../include/kernel.h"

#define TERM_COLS   80
#define TERM_ROWS   28
#define TERM_BUF    (TERM_COLS * TERM_ROWS)
#define CHAR_W      8
#define CHAR_H      16
#define INPUT_MAX   128

typedef struct {
    char  ch;
    u32   fg;
    u32   bg;
} term_cell_t;

static term_cell_t term_buf[TERM_ROWS][TERM_COLS];
static int cur_col = 0;
static int cur_row = 0;
static int term_ox, term_oy;  // pixel offset on screen

static void term_clear(void) {
    for (int r = 0; r < TERM_ROWS; r++)
        for (int c = 0; c < TERM_COLS; c++) {
            term_buf[r][c].ch = ' ';
            term_buf[r][c].fg = COLOR_TEXT_BRIGHT;
            term_buf[r][c].bg = 0x00050F18;
        }
    cur_col = cur_row = 0;
}

static void term_render_cell(int row, int col) {
    int px = term_ox + col * CHAR_W;
    int py = term_oy + row * CHAR_H;
    fb_draw_char(px, py, term_buf[row][col].ch,
        term_buf[row][col].fg, term_buf[row][col].bg, 1);
}

static void term_render_all(void) {
    for (int r = 0; r < TERM_ROWS; r++)
        for (int c = 0; c < TERM_COLS; c++)
            term_render_cell(r, c);
}

static void term_scroll(void) {
    for (int r = 0; r < TERM_ROWS - 1; r++)
        for (int c = 0; c < TERM_COLS; c++)
            term_buf[r][c] = term_buf[r+1][c];
    for (int c = 0; c < TERM_COLS; c++) {
        term_buf[TERM_ROWS-1][c].ch = ' ';
        term_buf[TERM_ROWS-1][c].fg = COLOR_TEXT_BRIGHT;
        term_buf[TERM_ROWS-1][c].bg = 0x00050F18;
    }
    cur_row = TERM_ROWS - 1;
    cur_col = 0;
    term_render_all();
}

static void term_newline(void) {
    cur_col = 0;
    cur_row++;
    if (cur_row >= TERM_ROWS) term_scroll();
}

static void term_putchar(char c, u32 fg) {
    if (c == '\n') { term_newline(); return; }
    if (c == '\r') { cur_col = 0; return; }
    if (c == '\t') {
        int next = (cur_col + 4) & ~3;
        while (cur_col < next) term_putchar(' ', fg);
        return;
    }
    if (cur_col >= TERM_COLS) term_newline();
    term_buf[cur_row][cur_col].ch = c;
    term_buf[cur_row][cur_col].fg = fg;
    term_render_cell(cur_row, cur_col);
    cur_col++;
}

static void term_puts(const char *s, u32 fg) {
    while (*s) term_putchar(*s++, fg);
}

static void term_puts_ln(const char *s, u32 fg) {
    term_puts(s, fg);
    term_newline();
}

// Wait for a line of input
static int term_readline(char *buf, int max) {
    int len = 0;
    int save_row = cur_row;
    int save_col = cur_col;

    while (1) {
        // Draw cursor
        int px = term_ox + cur_col * CHAR_W;
        int py = term_oy + cur_row * CHAR_H;
        fb_fill_rect(px, py + CHAR_H - 2, CHAR_W, 2, COLOR_ARCTIC_ACC);

        char c = keyboard_getchar();

        // Erase cursor
        fb_fill_rect(px, py + CHAR_H - 2, CHAR_W, 2,
            term_buf[cur_row][cur_col].bg);

        if (c == '\n' || c == '\r') {
            buf[len] = '\0';
            term_newline();
            return len;
        } else if (c == '\b') {
            if (len > 0) {
                len--;
                cur_col--;
                if (cur_col < 0) cur_col = 0;
                term_buf[cur_row][cur_col].ch = ' ';
                term_render_cell(cur_row, cur_col);
            }
        } else if (c == 0x03) { // Ctrl+C
            buf[0] = '\0';
            term_newline();
            return -1;
        } else if (len < max - 1 && c >= 0x20 && c < 0x7F) {
            buf[len++] = c;
            term_putchar(c, COLOR_TEXT_BRIGHT);
        }
        (void)save_row; (void)save_col;
    }
}

// ============================================================
// SHELL COMMANDS
// ============================================================
static void cmd_help(void) {
    term_puts_ln("", 0);
    term_puts_ln("=== ArcticOS Shell v1.0 ===", COLOR_ARCTIC_ACC);
    term_puts_ln("Available commands:", COLOR_YELLOW);
    term_puts_ln("  help     - show this help", COLOR_TEXT_BRIGHT);
    term_puts_ln("  clear    - clear the screen", COLOR_TEXT_BRIGHT);
    term_puts_ln("  time     - show current time and date", COLOR_TEXT_BRIGHT);
    term_puts_ln("  uname    - system info", COLOR_TEXT_BRIGHT);
    term_puts_ln("  echo ... - print text", COLOR_TEXT_BRIGHT);
    term_puts_ln("  meminfo  - memory info", COLOR_TEXT_BRIGHT);
    term_puts_ln("  cpuid    - CPU info", COLOR_TEXT_BRIGHT);
    term_puts_ln("  uptime   - system uptime", COLOR_TEXT_BRIGHT);
    term_puts_ln("  color    - color test", COLOR_TEXT_BRIGHT);
    term_puts_ln("  exit     - return to desktop", COLOR_TEXT_BRIGHT);
    term_puts_ln("", 0);
}

static void cmd_time(void) {
    rtc_time_t t;
    rtc_read(&t);
    char buf[64];
    ksprintf(buf, "Date: %s %02d %s %u",
        rtc_weekday_str(t.weekday), (u32)t.day,
        rtc_month_str(t.month), (u32)t.year);
    term_puts_ln(buf, COLOR_ARCTIC_ACC);
    ksprintf(buf, "Time: %02d:%02d:%02d",
        (u32)t.hour, (u32)t.minute, (u32)t.second);
    term_puts_ln(buf, COLOR_ARCTIC_ACC);
}

static void cmd_uname(void) {
    term_puts_ln("ArcticOS 1.0.0 (i386) [C + ASM] " __DATE__, COLOR_GREEN);
    term_puts_ln("Kernel: ArcticOS Monolithic Kernel", COLOR_TEXT_BRIGHT);
    term_puts_ln("Arch: i686 (x86 32-bit Protected Mode)", COLOR_TEXT_BRIGHT);
    term_puts_ln("Boot: GRUB2 Multiboot", COLOR_TEXT_BRIGHT);
    term_puts_ln("Display: VESA VBE Framebuffer", COLOR_TEXT_BRIGHT);
}

static void cmd_meminfo(void) {
    char buf[64];
    ksprintf(buf, "Framebuffer: %ux%u @ %u bpp",
        fb.width, fb.height, (u32)fb.bpp);
    term_puts_ln(buf, COLOR_TEXT_BRIGHT);
    ksprintf(buf, "FB address: 0x%x", (u32)(u32*)fb.addr);
    term_puts_ln(buf, COLOR_TEXT_BRIGHT);
    ksprintf(buf, "Pitch: %u bytes", fb.pitch);
    term_puts_ln(buf, COLOR_TEXT_BRIGHT);
    term_puts_ln("RAM: detected by GRUB (no MMU)", COLOR_LIGHT_GRAY);
}

static void cmd_cpuid(void) {
    u32 eax, ebx, ecx, edx;
    char vendor[13];
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    *(u32*)(vendor)     = ebx;
    *(u32*)(vendor + 4) = edx;
    *(u32*)(vendor + 8) = ecx;
    vendor[12] = '\0';
    char buf[64];
    ksprintf(buf, "Vendor: %s", vendor);
    term_puts_ln(buf, COLOR_TEXT_BRIGHT);
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    ksprintf(buf, "Model: family=%u model=%u stepping=%u",
        (eax >> 8) & 0xF, (eax >> 4) & 0xF, eax & 0xF);
    term_puts_ln(buf, COLOR_TEXT_BRIGHT);
    ksprintf(buf, "Features: %s %s %s",
        (edx >> 4) & 1 ? "TSC" : "",
        (edx >> 23) & 1 ? "MMX" : "",
        (edx >> 25) & 1 ? "SSE" : "");
    term_puts_ln(buf, COLOR_TEXT_BRIGHT);
}

static void cmd_uptime(void) {
    u32 ticks = timer_get_ticks();
    u32 secs  = ticks / 100;
    u32 mins  = secs / 60;
    u32 hrs   = mins / 60;
    char buf[64];
    ksprintf(buf, "Uptime: %u hrs %u min %u sec", hrs, mins % 60, secs % 60);
    term_puts_ln(buf, COLOR_TEXT_BRIGHT);
}

static void cmd_color(void) {
    term_puts_ln("Terminal color test:", COLOR_WHITE);
    u32 colors[] = { 0xFF0000, 0xFF8800, 0xFFFF00, 0x00FF00,
                     0x00FFFF, 0x0088FF, 0x8800FF, 0xFF00FF };
    const char *names[] = { "Red", "Orange", "Yellow", "Green",
                             "Cyan", "Blue", "Purple", "Pink" };
    for (int i = 0; i < 8; i++) {
        term_puts("  ", COLOR_WHITE);
        term_puts_ln(names[i], colors[i]);
    }
}

// ============================================================
// MAIN TERMINAL LOOP
// ============================================================
void app_terminal_run(void) {
    // Window
    int win_x = 40, win_y = 20;
    int win_w = fb.width - 80;
    int win_h = fb.height - 70;

    fb_fill_rect(win_x, win_y, win_w, win_h, 0x00050F18);
    fb_draw_rect(win_x, win_y, win_w, win_h, COLOR_ARCTIC_ACC, 2);
    fb_fill_rect(win_x, win_y, win_w, 26, COLOR_ARCTIC_BAR);
    fb_draw_string(win_x + 10, win_y + 6, "Terminal - ArcticOS Shell", COLOR_ARCTIC_ACC, COLOR_ARCTIC_BAR, 1);
    fb_draw_string(win_x + win_w - 64, win_y + 6, "[exit]=Quit", COLOR_LIGHT_GRAY, COLOR_ARCTIC_BAR, 1);

    term_ox = win_x + 4;
    term_oy = win_y + 30;

    term_clear();
    term_render_all();

    // Welcome message
    term_puts_ln("ArcticOS Shell v1.0 - Type 'help' to see commands", COLOR_ARCTIC_ACC);
    term_puts_ln("", 0);

    char input[INPUT_MAX];
    char prompt[] = "arctic@os:~$ ";

    while (1) {
        term_puts(prompt, COLOR_GREEN);
        int ret = term_readline(input, INPUT_MAX);
        if (ret < 0) {
            term_puts_ln("^C", COLOR_RED);
            continue;
        }
        if (ret == 0) continue;

        // Command processing
        if (kstrcmp(input, "help") == 0) {
            cmd_help();
        } else if (kstrcmp(input, "clear") == 0) {
            term_clear();
            term_render_all();
        } else if (kstrcmp(input, "time") == 0 || kstrcmp(input, "date") == 0) {
            cmd_time();
        } else if (kstrcmp(input, "uname") == 0 || kstrcmp(input, "uname -a") == 0) {
            cmd_uname();
        } else if (kstrcmp(input, "meminfo") == 0) {
            cmd_meminfo();
        } else if (kstrcmp(input, "cpuid") == 0) {
            cmd_cpuid();
        } else if (kstrcmp(input, "uptime") == 0) {
            cmd_uptime();
        } else if (kstrcmp(input, "color") == 0) {
            cmd_color();
        } else if (kstrcmp(input, "exit") == 0 || kstrcmp(input, "quit") == 0) {
            break;
        } else if (kstrncmp(input, "echo ", 5) == 0) {
            term_puts_ln(input + 5, COLOR_TEXT_BRIGHT);
        } else {
            char err[64];
            ksprintf(err, "Unknown command: '%s' (type 'help')", input);
            term_puts_ln(err, 0x00FF4444);
        }
    }
}
