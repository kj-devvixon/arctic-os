#include "../include/kernel.h"

#define ED_ROWS   25
#define ED_COLS   72
#define ED_LINES  200

static char ed_lines[ED_LINES][ED_COLS + 1];
static int  ed_line_len[ED_LINES];
static int  ed_total_lines = 1;
static int  ed_cur_row = 0;   // cursor: buffer row
static int  ed_cur_col = 0;   // cursor: column
static int  ed_scroll  = 0;   // first visible row
static int  ed_ox, ed_oy;     // pixel offset
static bool ed_modified = false;
static char ed_filename[64] = "note.txt";
static char ed_status[128]  = "";

// ============================================================
// EDITOR RENDERING
// ============================================================
static void ed_render_line(int buf_row) {
    int screen_row = buf_row - ed_scroll;
    if (screen_row < 0 || screen_row >= ED_ROWS) return;

    int py = ed_oy + screen_row * 16;
    int px = ed_ox + 40; // +40 for line numbers

    // Line number
    char num_s[8];
    kitoa(buf_row + 1, num_s, 10);
    fb_fill_rect(ed_ox, py, 38, 16, 0x00080F18);
    fb_draw_string(ed_ox + 2, py, num_s, 0x00446688, 0x00080F18, 1);

    // Line content
    char display[ED_COLS + 2];
    int len = ed_line_len[buf_row];
    kmemcpy(display, ed_lines[buf_row], len);
    // Pad with spaces to ED_COLS
    for (int i = len; i < ED_COLS; i++) display[i] = ' ';
    display[ED_COLS] = '\0';

    // Draw characters
    bool is_current = (buf_row == ed_cur_row);
    for (int col = 0; col < ED_COLS; col++) {
        u32 bg = 0x00050F18;
        u32 fg = COLOR_TEXT_BRIGHT;
        // Highlight current row
        if (is_current) bg = 0x000A1A28;
        // Cursor
        if (is_current && col == ed_cur_col) {
            bg = COLOR_ARCTIC_ACC;
            fg = COLOR_ARCTIC_BG;
        }
        fb_draw_char(px + col * 8, py, display[col], fg, bg, 1);
    }
}

static void ed_render_all(void) {
    for (int r = ed_scroll; r < ed_scroll + ED_ROWS && r < ed_total_lines; r++)
        ed_render_line(r);
    // Fill empty rows
    for (int r = ed_total_lines; r < ed_scroll + ED_ROWS; r++) {
        int sy = ed_oy + (r - ed_scroll) * 16;
        if (sy >= ed_oy + ED_ROWS * 16) break;
        fb_fill_rect(ed_ox, sy, ED_COLS * 8 + 40, 16, 0x00050F18);
        fb_draw_char(ed_ox + 42, sy, '~', 0x00334455, 0x00050F18, 1);
    }
}

static void ed_update_status(void) {
    ksprintf(ed_status, "%s%s | Ln %d/%d Col %d | [F1] Help [ESC] Exit",
        ed_filename,
        ed_modified ? " [*]" : "",
        ed_cur_row + 1, ed_total_lines, ed_cur_col + 1);

    // Status bar at bottom
    int sy = ed_oy + ED_ROWS * 16 + 2;
    fb_fill_rect(ed_ox - 2, sy, ED_COLS * 8 + 44, 16, COLOR_ARCTIC_BAR);
    fb_draw_string(ed_ox, sy, ed_status, COLOR_TEXT_BRIGHT, COLOR_ARCTIC_BAR, 1);
}

static void ed_ensure_visible(void) {
    if (ed_cur_row < ed_scroll) {
        ed_scroll = ed_cur_row;
        ed_render_all();
    } else if (ed_cur_row >= ed_scroll + ED_ROWS) {
        ed_scroll = ed_cur_row - ED_ROWS + 1;
        ed_render_all();
    }
}

// ============================================================
// TEXT OPERATIONS
// ============================================================
static void ed_insert_char(char c) {
    if (ed_cur_col >= ED_COLS) return;
    char *line = ed_lines[ed_cur_row];
    int   len  = ed_line_len[ed_cur_row];
    // Shift characters right
    for (int i = len; i > ed_cur_col; i--)
        line[i] = line[i-1];
    line[ed_cur_col] = c;
    ed_line_len[ed_cur_row]++;
    ed_cur_col++;
    ed_modified = true;
}

static void ed_delete_char(void) {
    char *line = ed_lines[ed_cur_row];
    int   len  = ed_line_len[ed_cur_row];
    if (ed_cur_col == 0) {
        // Merge with previous line
        if (ed_cur_row == 0) return;
        int prev_len = ed_line_len[ed_cur_row - 1];
        char *prev = ed_lines[ed_cur_row - 1];
        if (prev_len + len <= ED_COLS) {
            kmemcpy(prev + prev_len, line, len);
            ed_line_len[ed_cur_row - 1] = prev_len + len;
            prev[prev_len + len] = '\0';
            // Delete current line
            for (int r = ed_cur_row; r < ed_total_lines - 1; r++) {
                kmemcpy(ed_lines[r], ed_lines[r+1], ed_line_len[r+1]);
                ed_line_len[r] = ed_line_len[r+1];
            }
            ed_total_lines--;
            ed_cur_row--;
            ed_cur_col = prev_len;
            ed_render_all();
        }
    } else {
        // Delete character before cursor
        for (int i = ed_cur_col - 1; i < len - 1; i++)
            line[i] = line[i+1];
        ed_line_len[ed_cur_row]--;
        ed_cur_col--;
        ed_render_line(ed_cur_row);
    }
    ed_modified = true;
}

static void ed_newline(void) {
    if (ed_total_lines >= ED_LINES) return;
    char *line = ed_lines[ed_cur_row];
    int   len  = ed_line_len[ed_cur_row];
    // Shift lines down
    for (int r = ed_total_lines; r > ed_cur_row + 1; r--) {
        kmemcpy(ed_lines[r], ed_lines[r-1], ed_line_len[r-1]);
        ed_line_len[r] = ed_line_len[r-1];
    }
    // New line = rest of current line
    int rest = len - ed_cur_col;
    kmemcpy(ed_lines[ed_cur_row + 1], line + ed_cur_col, rest);
    ed_line_len[ed_cur_row + 1] = rest;
    // Truncate current line
    ed_line_len[ed_cur_row] = ed_cur_col;
    ed_total_lines++;
    ed_cur_row++;
    ed_cur_col = 0;
    ed_modified = true;
    ed_render_all();
}

// ============================================================
// MAIN EDITOR LOOP
// ============================================================
void app_editor_run(void) {
    // Initialize buffer
    kmemset(ed_lines, 0, sizeof(ed_lines));
    kmemset(ed_line_len, 0, sizeof(ed_line_len));
    ed_total_lines = 1;
    ed_cur_row = ed_cur_col = ed_scroll = 0;
    ed_modified = false;

    // Insert welcome text
    const char *welcome = "Welcome to ArcticOS Editor!";
    kstrcpy(ed_lines[0], welcome);
    ed_line_len[0] = kstrlen(welcome);
    kstrcpy(ed_lines[1], "");
    ed_line_len[1] = 0;
    kstrcpy(ed_lines[2], "Keyboard shortcuts:");
    ed_line_len[2] = kstrlen(ed_lines[2]);
    kstrcpy(ed_lines[3], "  Arrow keys - move cursor");
    ed_line_len[3] = kstrlen(ed_lines[3]);
    kstrcpy(ed_lines[4], "  Home/End   - start/end of line");
    ed_line_len[4] = kstrlen(ed_lines[4]);
    kstrcpy(ed_lines[5], "  PgUp/PgDn  - page up/down");
    ed_line_len[5] = kstrlen(ed_lines[5]);
    kstrcpy(ed_lines[6], "  ESC        - return to desktop");
    ed_line_len[6] = kstrlen(ed_lines[6]);
    kstrcpy(ed_lines[7], "");
    ed_line_len[7] = 0;
    kstrcpy(ed_lines[8], "Start typing below...");
    ed_line_len[8] = kstrlen(ed_lines[8]);
    ed_total_lines = 10;

    // Window
    int win_x = 20, win_y = 20;
    int win_w = fb.width - 40;
    int win_h = fb.height - 60;

    fb_fill_rect(win_x, win_y, win_w, win_h, 0x00050F18);
    fb_draw_rect(win_x, win_y, win_w, win_h, COLOR_ARCTIC_ACC, 2);

    // Title bar
    fb_fill_rect(win_x, win_y, win_w, 26, COLOR_ARCTIC_BAR);
    fb_draw_string(win_x + 10, win_y + 6, "Text Editor - ArcticOS", COLOR_ARCTIC_ACC, COLOR_ARCTIC_BAR, 1);
    fb_draw_string(win_x + win_w - 64, win_y + 6, "[ESC]=Exit", COLOR_LIGHT_GRAY, COLOR_ARCTIC_BAR, 1);

    ed_ox = win_x + 4;
    ed_oy = win_y + 30;

    ed_render_all();
    ed_update_status();

    // Main loop
    while (1) {
        // Refresh current row (cursor)
        ed_render_line(ed_cur_row);
        ed_update_status();

        char c = keyboard_getchar();

        if (c == 27) break; // ESC

        // Navigation
        else if (c == 0) {
            // Extended keys - second byte
        }
        else if (c == 'A' - 64 + 1) { // Ctrl+A = Home
            ed_cur_col = 0;
        }
        else if (c == 'E' - 64 + 1) { // Ctrl+E = End
            ed_cur_col = ed_line_len[ed_cur_row];
        }
        else if (c == '\n' || c == '\r') {
            ed_newline();
            ed_ensure_visible();
        }
        else if (c == '\b') {
            ed_delete_char();
            ed_ensure_visible();
        }
        else if (c >= 0x20 && c < 0x7F) {
            ed_insert_char(c);
            // Visual line wrap
            if (ed_cur_col > ED_COLS - 1) {
                ed_newline();
            }
            ed_render_line(ed_cur_row);
            ed_ensure_visible();
        }
    }
}
