#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ============================================================
// BASIC TYPES
// ============================================================
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

// ============================================================
// MULTIBOOT STRUCTURES
// ============================================================
#define MBOOT1_MAGIC 0x2BADB002
#define MBOOT2_MAGIC 0x36D76289

// --- Multiboot 1 ---
typedef struct {
    u32 flags;
    u32 mem_lower;
    u32 mem_upper;
    u32 boot_device;
    u32 cmdline;
    u32 mods_count;
    u32 mods_addr;
    u32 syms[4];
    u32 mmap_length;
    u32 mmap_addr;
    u32 drives_length;
    u32 drives_addr;
    u32 config_table;
    u32 boot_loader_name;
    u32 apm_table;
    u32 vbe_control_info;
    u32 vbe_mode_info;
    u16 vbe_mode;
    u16 vbe_interface_seg;
    u16 vbe_interface_off;
    u16 vbe_interface_len;
    u64 framebuffer_addr;
    u32 framebuffer_pitch;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u8  framebuffer_bpp;
    u8  framebuffer_type;
} __attribute__((packed)) multiboot_info_t;

// --- Multiboot 2 ---
typedef struct {
    u32 total_size;
    u32 reserved;
} __attribute__((packed)) mb2_info_t;

typedef struct {
    u32 type;
    u32 size;
} __attribute__((packed)) mb2_tag_t;

typedef struct {
    u32 type;       // = 8
    u32 size;
    u64 framebuffer_addr;
    u32 framebuffer_pitch;
    u32 framebuffer_width;
    u32 framebuffer_height;
    u8  framebuffer_bpp;
    u8  framebuffer_type;
    u16 reserved;
} __attribute__((packed)) mb2_tag_framebuffer_t;

// ============================================================
// FRAMEBUFFER
// ============================================================
typedef struct {
    u32 *addr;
    u32  pitch;
    u32  width;
    u32  height;
    u8   bpp;
    u32  pitch_pixels;
} framebuffer_t;

extern framebuffer_t fb;

// ============================================================
// COLORS (32-bit ARGB)
// ============================================================
#define COLOR_BLACK       0x00000000
#define COLOR_WHITE       0x00FFFFFF
#define COLOR_RED         0x00FF0000
#define COLOR_GREEN       0x0000FF00
#define COLOR_BLUE        0x000088FF
#define COLOR_CYAN        0x0000FFFF
#define COLOR_YELLOW      0x00FFFF00
#define COLOR_MAGENTA     0x00FF00FF
#define COLOR_DARK_GRAY   0x00222222
#define COLOR_LIGHT_GRAY  0x00AAAAAA
#define COLOR_ARCTIC_BG   0x00051020
#define COLOR_ARCTIC_BAR  0x00102040
#define COLOR_ARCTIC_BTN  0x001A3A6A
#define COLOR_ARCTIC_ACC  0x0000AAFF
#define COLOR_ARCTIC_WIN  0x000D1F3C
#define COLOR_TEXT_BRIGHT 0x00E0F0FF

// ============================================================
// ASM FUNCTION DECLARATIONS
// ============================================================
extern void outb(u16 port, u8 val);
extern u8   inb(u16 port);
extern void outw(u16 port, u16 val);
extern u16  inw(u16 port);
extern void enable_interrupts(void);
extern void disable_interrupts(void);
extern void idt_load(void *idt_ptr);
extern void gdt_load(void *gdt_ptr);

// ============================================================
// MODULE DECLARATIONS
// ============================================================

// GDT/IDT
void gdt_init(void);
void idt_init(void);

// Interrupts
void pic_init(void);
void irq_handler(int irq_num);
void isr_handler(int isr_num);

// Framebuffer / graphics
void fb_init(multiboot_info_t *mbi);
void fb_init_mb2(mb2_info_t *mb2);
void fb_clear(u32 color);
void fb_put_pixel(int x, int y, u32 color);
void fb_fill_rect(int x, int y, int w, int h, u32 color);
void fb_draw_rect(int x, int y, int w, int h, u32 color, int thickness);
void fb_draw_char(int x, int y, char c, u32 fg, u32 bg, int scale);
void fb_draw_string(int x, int y, const char *s, u32 fg, u32 bg, int scale);
void fb_draw_line(int x0, int y0, int x1, int y1, u32 color);
void fb_fill_circle(int cx, int cy, int r, u32 color);

// PS/2 Mouse
void mouse_init(void);
void mouse_handler(void);
int  mouse_get_x(void);
int  mouse_get_y(void);
bool mouse_get_left(void);
bool mouse_get_right(void);
bool mouse_pop_click(void);  // returns true and clears flag if LMB was clicked
void mouse_draw_cursor(void);  // refresh cursor after screen redraw
void keyboard_init(void);
char keyboard_getchar(void);
bool keyboard_has_char(void);
void keyboard_handler(void);
const char *keyboard_get_buffer(void);

// RTC
typedef struct {
    u8 second;
    u8 minute;
    u8 hour;
    u8 day;
    u8 month;
    u16 year;
    u8 weekday;
} rtc_time_t;

void rtc_init(void);
void rtc_read(rtc_time_t *t);
void rtc_handler(void);
const char *rtc_weekday_str(u8 wd);
const char *rtc_month_str(u8 m);

// Timer
void timer_init(u32 freq);
u32  timer_get_ticks(void);
void timer_sleep(u32 ms);

// Desktop
void desktop_init(void);
void desktop_run(void);
void desktop_draw(void);

// Apps
void app_clock_run(void);
void app_terminal_run(void);
void app_editor_run(void);

// libc
int    kstrlen(const char *s);
char  *kstrcpy(char *dst, const char *src);
char  *kstrcat(char *dst, const char *src);
int    kstrcmp(const char *a, const char *b);
int    kstrncmp(const char *a, const char *b, int n);
void  *kmemset(void *ptr, int val, size_t n);
void  *kmemcpy(void *dst, const void *src, size_t n);
void   kitoa(i32 val, char *buf, int base);
void   kutoa(u32 val, char *buf, int base);
int    katoi(const char *s);
void   ksprintf(char *buf, const char *fmt, ...);

#endif // KERNEL_H
