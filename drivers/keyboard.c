// ============================================================
// ArcticOS - PS/2 Keyboard Driver
// IRQ1 handling, circular buffer, scancode set 1 -> ASCII
// ============================================================

#include "../include/kernel.h"

// ============================================================
// PS/2 PORTS
// ============================================================
#define KBD_DATA_PORT    0x60
#define KBD_STATUS_PORT  0x64

// ============================================================
// CIRCULAR BUFFER
// ============================================================
#define KBD_BUFFER_SIZE  64

static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile int kbd_head = 0;  // write (ISR)
static volatile int kbd_tail = 0;  // read (kernel)

static void buffer_push(char c) {
    int next = (kbd_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

// ============================================================
// SCANCODE -> ASCII MAP (Scancode Set 1, US QWERTY)
// ============================================================
static const char scancode_map_normal[128] = {
    0,    // 0x00
    0,    // 0x01 - Escape
    '1','2','3','4','5','6','7','8','9','0','-','=',
    '\b', // 0x0E - Backspace
    '\t', // 0x0F - Tab
    'q','w','e','r','t','y','u','i','o','p','[',']',
    '\n', // 0x1C - Enter
    0,    // 0x1D - LCtrl
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,    // 0x2A - LShift
    '\\',
    'z','x','c','v','b','n','m',',','.','/',
    0,    // 0x36 - RShift
    '*',  // 0x37
    0,    // 0x38 - LAlt
    ' ',  // 0x39 - Space
    0,    // 0x3A - CapsLock
    0,0,0,0,0,0,0,0,0,0, // F1-F10
    0,    // NumLock
    0,    // ScrollLock
    '7','8','9','-','4','5','6','+','1','2','3','0','.', // Numpad
    0,0,0,0,0
};

static const char scancode_map_shift[128] = {
    0,
    0,    // Escape
    '!','@','#','$','%','^','&','*','(',')','_','+',
    '\b',
    '\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}',
    '\n',
    0,    // LCtrl
    'A','S','D','F','G','H','J','K','L',':','"','~',
    0,    // LShift
    '|',
    'Z','X','C','V','B','N','M','<','>','?',
    0,    // RShift
    '*',
    0,    // LAlt
    ' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

// ============================================================
// MODIFIER STATE
// ============================================================
static volatile bool shift_pressed = false;
static volatile bool capslock_on   = false;

// ============================================================
// IRQ1 INTERRUPT HANDLER
// ============================================================
void keyboard_handler(void) {
    u8 sc = inb(KBD_DATA_PORT);

    bool released = (sc & 0x80) != 0;
    u8 key = sc & 0x7F;

    if (key == 0x2A || key == 0x36) {
        shift_pressed = !released;
        return;
    }
    if (key == 0x3A && !released) {
        capslock_on = !capslock_on;
        return;
    }
    if (key == 0x01 && !released) {
        buffer_push(0x1B); // ESC
        return;
    }

    if (released) return;
    if (key >= 128) return;

    char c = shift_pressed ? scancode_map_shift[key] : scancode_map_normal[key];
    if (c == 0) return;

    if (capslock_on && c >= 'a' && c <= 'z') c -= 32;
    else if (capslock_on && c >= 'A' && c <= 'Z') c += 32;

    buffer_push(c);
}

// ============================================================
// INITIALIZATION
// ============================================================
void keyboard_init(void) {
    while (inb(KBD_STATUS_PORT) & 0x01)
        inb(KBD_DATA_PORT);
    kbd_head = 0;
    kbd_tail = 0;
    shift_pressed = false;
    capslock_on   = false;
}

// ============================================================
// PUBLIC API
// ============================================================
bool keyboard_has_char(void) {
    return kbd_head != kbd_tail;
}

char keyboard_getchar(void) {
    while (!keyboard_has_char())
        __asm__ volatile("hlt");
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

const char *keyboard_get_buffer(void) {
    return kbd_buffer;
}
