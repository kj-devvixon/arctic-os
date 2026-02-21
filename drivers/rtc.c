#include "../include/kernel.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define RTC_SECONDS   0x00
#define RTC_MINUTES   0x02
#define RTC_HOURS     0x04
#define RTC_WEEKDAY   0x06
#define RTC_DAY       0x07
#define RTC_MONTH     0x08
#define RTC_YEAR      0x09
#define RTC_CENTURY   0x32
#define RTC_STATUS_A  0x0A
#define RTC_STATUS_B  0x0B
#define RTC_STATUS_C  0x0C

static rtc_time_t current_time;
static bool rtc_updated = false;

static u8 cmos_read(u8 reg) {
    outb(CMOS_ADDR, reg | 0x80); // bit 7 = disable NMI during read
    return inb(CMOS_DATA);
}

static bool rtc_is_updating(void) {
    outb(CMOS_ADDR, RTC_STATUS_A);
    return (inb(CMOS_DATA) & 0x80) != 0;
}

static u8 bcd_to_bin(u8 val) {
    return (val & 0x0F) + ((val >> 4) * 10);
}

void rtc_init(void) {
    // Enable RTC interrupts (IRQ8) - update every second
    u8 prev = cmos_read(RTC_STATUS_B);
    outb(CMOS_ADDR, RTC_STATUS_B | 0x80);
    outb(CMOS_DATA, prev | 0x10); // bit 4 = Update-ended interrupt
    // Read once on startup
    rtc_read(&current_time);
}

void rtc_read(rtc_time_t *t) {
    // Wait until RTC is not updating
    while (rtc_is_updating());

    t->second  = cmos_read(RTC_SECONDS);
    t->minute  = cmos_read(RTC_MINUTES);
    t->hour    = cmos_read(RTC_HOURS);
    t->weekday = cmos_read(RTC_WEEKDAY);
    t->day     = cmos_read(RTC_DAY);
    t->month   = cmos_read(RTC_MONTH);
    t->year    = cmos_read(RTC_YEAR);
    u8 century = cmos_read(RTC_CENTURY);
    u8 status_b = cmos_read(RTC_STATUS_B);

    // BCD -> binary conversion (if BCD mode)
    if (!(status_b & 0x04)) {
        t->second  = bcd_to_bin(t->second);
        t->minute  = bcd_to_bin(t->minute);
        t->hour    = bcd_to_bin(t->hour) | (t->hour & 0x80);
        t->weekday = bcd_to_bin(t->weekday);
        t->day     = bcd_to_bin(t->day);
        t->month   = bcd_to_bin(t->month);
        t->year    = bcd_to_bin(t->year);
        century    = bcd_to_bin(century);
    }

    // 12h -> 24h
    if (!(status_b & 0x02) && (t->hour & 0x80)) {
        t->hour = ((t->hour & 0x7F) + 12) % 24;
    }

    // Full year
    if (century != 0) {
        t->year += century * 100;
    } else {
        t->year += (t->year < 70) ? 2000 : 1900;
    }

    if (t->weekday == 0) t->weekday = 7;
    rtc_updated = true;
}

void rtc_handler(void) {
    // Read Status C to acknowledge interrupt
    outb(CMOS_ADDR, RTC_STATUS_C);
    inb(CMOS_DATA);
    rtc_read(&current_time);
}

const char *rtc_weekday_str(u8 wd) {
    static const char *days[] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    if (wd < 1 || wd > 7) return "???";
    return days[wd];
}

const char *rtc_month_str(u8 m) {
    static const char *months[] = {"", "Jan", "Feb", "Mar", "Apr",
        "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (m < 1 || m > 12) return "???";
    return months[m];
}
