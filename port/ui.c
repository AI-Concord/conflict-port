// User-interface helpers (original code segment 1000, first module).
// Coordinates are given on a 640x200 grid; y values are scaled by 1.75 to the
// 350-line EGA screen exactly as the original did with floating point.
#include "conflict.h"
#include <stdlib.h>

#define FONT_A     UW(0xb294)       // etech.fnt
#define FONT_B     UW(0xb296)       // extra.fnt (symbols)
#define UI_CONTEXT W(0xb270)        // which screen the menu belongs to (remembers the last choice)
#define MENU_LINES W(0x87b0)        // text lines used so far in the menu being built
#define MENU_COUNT W(0x87c8)        // number of selectable options
#define MENU_LINE(i) W(0x87b4 + (i) * 2)    // line index of option i (1-based)
#define MENU_LAST(c) W(0x1200 + ((c) - 1) * 2)

int game_rand(void) {                              // 20CA:0003
    u32 seed = ((u32)UW(0x7ff2) << 16) | UW(0x7ff0);
    seed = seed * 0x41c64e6du + 0x3039u;
    UW(0x7ff2) = (u16)(seed >> 16); UW(0x7ff0) = (u16)seed;
    return (int)((seed >> 16) & 0x7fff);
}
void game_srand(u16 seed) { UW(0x7ff2) = 0; UW(0x7ff0) = seed; }      // 20CA:002A

int zt_fgetc(FILE *f) {                            // 21AE:0059, text mode: CR dropped, Ctrl-Z ends the file
    int c;
    do { c = fgetc(f); if (c == EOF) return -1; } while (c == 0x0d);
    if (c == 0x1a) { fseek(f, 0, SEEK_END); return -1; }
    return c;
}

int rnd(int n) {                                   // 1000:0010 - random number 1..n
    if (n < 0) return 0;
    return (int)((double)game_rand() * (double)n / 32768.0 + 1.0);
}

void draw_box(int x1, int y1, int x2, int y2) {    // 1000:00B3 - filled panel with a drop shadow
    u8 save = G_COLOR;
    G_COLOR = G_BGCOLOR;
    double dy2 = (double)y2 * 1.75, dy1 = (double)y1 * 1.75;
    int py2 = (int)dy2, py1 = (int)dy1;
    gfx_fillrect(x1, x2, py1, py2);
    G_COLOR = save;
    int top = (int)(dy1 - 1.0);
    gfx_hline(x1 - 1, x2 + 1, top, 0);
    int bot = (int)(dy2 + 1.0);
    gfx_hline(x1 - 1, x2 + 1, bot, 0);
    gfx_hline(x1, x2 + 2, (int)(dy2 + 2.0), 0);
    int bot3 = (int)(dy2 + 3.0);
    gfx_hline(x1, x2 + 2, bot3, 0);
    gfx_vline(top, bot, x1 - 1, 0);
    gfx_vline(top, bot, x2 + 1, 0);
    gfx_vline(top, bot, x2 + 2, 0);
    gfx_vline(py1, bot3, x2 + 3, 0);
}

void draw_titled_box(int x1, int y1, int x2, int y2, const char *title) {     // 1000:0286
    draw_box(x1, y1, x2, y2);
    int x = x1;
    int w = gfx_text_width(title);
    if (w < x2 - x1) x = x1 + ((x2 - x1 - w) >> 1);
    double dy = (double)y1 * 1.75;
    gfx_hline(x - 1, x + w + 1, (int)(dy + 9.0), 0);
    gfx_moveto(x, (int)(dy + 1.0));
    gfx_text(title);
}

int contains4(const char *s, const char *pat) {    // 1000:0357 - does s contain the 4 characters at pat?
    for (; *s; s++)
        if (s[0] == pat[0] && s[1] == pat[1] && s[2] == pat[2] && s[3] == pat[3]) return 1;
    return 0;
}

void str_overlay(char *dst, const char *src, int pos) {       // 1000:03BD - write src over dst at column pos (1-based)
    char *d = dst + pos - 1;
    while (*src) *d++ = *src++;
}

void squeeze_spaces(char *s) {                     // 1000:03E5 - collapse runs of spaces
    for (int i = 0; i < 100; i++) B(0x874c + i) = 0;
    int run = 0; char *d = s;
    for (; *s; s++) {
        if (*s == ' ') run++; else run = 0;
        if (run < 2) *d++ = *s;
    }
    *d = 0;
}

int wait_key(void) {                               // 1000:0444 - BIOS keyboard read
    UW(0xb2a4) = 0;
    u16 k = host_key_read();
    UW(0xb2a4) = k;
    return (i16)k;
}

void text_mode(void) {                             // 1000:0479
    W(0xb2b4) = 3;
    host_set_video_mode(3);
}

void show_status_chart(void) {                     // 1000:0491 - "Internal stability / Diplomatic relations" bars
    char buf[24];
    UI_CONTEXT = 0;
    G_FONT = FONT_A;
    strcpy(S(0xa1f2), S(0x110e));
    draw_titled_box(0x32, 5, 0x1ea, 0x73, S(0xa1f2));
    G_FONT = FONT_B;
    gfx_moveto(0x53, 0x16); gfx_text(S(0x113d));
    gfx_moveto(0x131, 0x16); gfx_text(S(0x114d));
    for (int i = 0; i < 7; i++) {
        G_FONT = FONT_A;
        const char *name = S(UW(0xd30 + i * 2));
        int w = gfx_text_width(name);
        int row = (int)(34.0 + (double)(i * 12) * 1.75);
        gfx_moveto((100 - w) / 2 + 0xcd, row);
        gfx_text(name);
        G_FONT = FONT_B;
        int n = 0;
        while ((double)W(0xa0ce + i * 2) * 1.5 > (double)n) { buf[n] = '='; n++; }
        buf[n] = '>';
        if (W(0x9380 + i * 2) != 0) { n = 0; buf[0] = 'X'; }
        if (W(0xa10e + i * 2) == 10) { n = 0; buf[0] = '<'; }
        buf[n + 1] = 0;
        gfx_moveto(0x131, row);
        gfx_text(buf);
        for (n = 0; n < 10 - W(0xa10e + i * 2); n++) buf[n + 1] = '=';
        buf[0] = '<';
        if (W(0xa10e + i * 2) == 10) buf[0] = '>';
        buf[n + 1] = 0;
        w = gfx_text_width(buf);
        gfx_moveto(0xcd - w, row);
        gfx_text(buf);
    }
    G_FONT = FONT_B;
    wait_key();
    gfx_copy_from_page(0x32, 0x1ea, 8, 0xc9);
    recolor_fallen();
    UI_CONTEXT = 1;
}

void calibrate_delay(void) {                       // 1000:0739 - the original timed a busy loop to scale its delays
    u32 total = 0;
    for (int i = 0; i < 20; i++) {
        int t0 = host_clock_cs();
        int t1 = host_clock_cs();
        total += (u32)(i32)(t1 - t0);
    }
    if (total < 0x28) total = 0x28;
    u16 avg = (u16)(total / 20);
    float f = (float)((double)(float)(double)avg / 17.0);
    memcpy(DSP + 0xb27a, &f, 4);
}

static void read_font(const char *name, u16 dest, int from, int to) {
    FILE *f = host_fopen(name, "rb");
    for (int i = from; i < to; i++) B(dest + i) = f ? (u8)zt_fgetc(f) : 0;
    if (f) fclose(f);
}

void load_fonts(void) {                            // 1000:07FC
    FONT_A = game_malloc(0x465);
    FONT_B = game_malloc(0x465);
    UW(0xb298) = game_malloc(0x3f0);
    UW(0xb29a) = game_malloc(0x3f0);
    UW(0xb29c) = game_malloc(0x900);
    if (UW(0xb29c) == 0 || UW(0xb29a) == 0 || UW(0xb298) == 0) {
        text_mode();
        puts(S(0x1160));
        host_exit();
    }
    UW(0xb29e) = UW(0xb29c);
    G_FONT = FONT_A;
    read_font(S(0x1187), FONT_A, 0x1b0, 0x466);         // etech.fnt: glyphs for '0'..
    read_font(S(0x11b3), FONT_B, 0x1b0, 0x466);         // extra.fnt
    read_font(S(0x11bf), UW(0xb298), 0, 0x3f0);         // xini.fnt   (16-pixel headline font)
    read_font(S(0x11ca), UW(0xb29a), 0, 0x3f0);         // xguard.fnt (16-pixel headline font)
    G_BIGFONT = (u16)(UW(0xb298) - 0x900);
    read_font(S(0x11d7), UW(0xb29c), 0, 0x380);         // xnclr.fnt: 32x32 icons
    read_font(S(0x11e3), UW(0xb29c), 0x380, 0x800);     // xtest.fnt: more icons
}

// 1000:0A7C - titled panel with word-wrapped body text, plus optional '|'-terminated extra lines.
void text_box(int x1, int y1, int x2, int y2, const char *title, const char *text, const char *extra) {
    char buf[80];
    int line = 0, drawn = 0;
    const char *p = text;
    int maxw = (x2 - x1) - 15;
    for (;;) {
        int w = (*p == 0) ? 0 : gfx_text_width(p);
        if (w <= maxw) break;
        int n = maxw / 7 + 10;
        while ((int)strlen(p) < n) n--;
        strncpy(buf, p, (size_t)n); buf[n] = 0;
        p = p + (signed char)n - 1;
        int again = 0;
        for (;;) {
            if (gfx_text_width(buf) <= maxw && again) break;
            for (;;) { p--; if (*p == ' ') break; n--; }
            n--; buf[n] = 0;
            again = 1;
        }
        p++;
        if (!drawn) { draw_titled_box(x1, y1, x2, y2, title); drawn = 1; }
        gfx_moveto(x1 + 15, (int)(((double)y1 * 1.75 + 16.0) + (double)line * 1.75));
        gfx_text(buf);
        line += 10;
    }
    if (!drawn) draw_titled_box(x1, y1, x2, y2, title);
    gfx_moveto(x1 + 15, (int)(((double)y1 * 1.75 + 16.0) + (double)line * 1.75));
    gfx_text(p);
    p = extra;
    for (;;) {
        line += 10;
        if (*extra == 0) break;
        int n = 1;
        while (*extra != '|') { n++; extra++; }
        extra++;
        strncpy(buf, p, (size_t)n); buf[n] = 0;
        p = extra;
        gfx_moveto(x1 + 15, (int)(((double)y1 * 1.75 + 16.0) + (double)line * 1.75));
        gfx_text(buf);
    }
}

// 1000:0D24 - build a menu from "Title?Option 1?Option 2?...?" and let the player choose.
int menu(int x1, int y1, int x2, int y2, const char *spec, int country) {
    char buf[80];
    MENU_LINES = 0;
    const char *p = spec;
    int w = (x2 - x1) - 25;
    for (;;) {
        int n = 0; const char *q = p;
        while (*q != '?') { n++; q++; }
        strncpy(buf, p, (size_t)n); buf[n] = 0;
        if (MENU_LINES == 0 && buf[0] == ' ') {
            str_overlay(buf, S(UW(0xd80 + country * 2)), 1);
            squeeze_spaces(buf);
        }
        if (MENU_LINES == 0) draw_titled_box(x1, y1, x2, y2, buf);
        else menu_item(x1, y1, w, buf);
        if (q[1] == 0) break;
        MENU_LINES++;
        p += (signed char)n + 1;
    }
    return menu_select(x2, y1);
}

// 1000:0E13 - add one (possibly wrapped) option line with its check box.
void menu_item(int x, int y, int w, const char *text) {
    char buf[80];
    const char *p = text;
    if (MENU_LINES == 1) MENU_COUNT = 0;
    for (;;) {
        if (gfx_text_width(p) <= w) break;
        int n = w / 7 + 10;
        while ((int)strlen(p) < n) n--;
        strncpy(buf, p, (size_t)n); buf[n] = 0;
        p = p + (signed char)n - 1;
        int again = 0;
        for (;;) {
            if (gfx_text_width(buf) <= w && again) break;
            for (;;) { p--; if (*p == ' ') break; n--; }
            n--; buf[n] = 0;
            again = 1;
        }
        p++;
        gfx_moveto(x + 10, (int)(((double)y * 1.75 + 13.0) + (double)(MENU_LINES << 3) * 1.75));
        MENU_LINES++;
        gfx_text(buf);
    }
    double base = (double)y * 1.75 + 13.0;
    gfx_moveto(x + 10, (int)(base + (double)(MENU_LINES << 3) * 1.75));
    i16 line = MENU_LINES;
    MENU_LINES++;
    MENU_COUNT++;
    MENU_LINE(MENU_COUNT) = line;
    gfx_text(p);
    gfx_moveto(x + w + 5, (int)(base + (double)((MENU_LINES - 1) * 8) * 1.75));
    G_FONT = FONT_B;
    gfx_text(S(0x11ed));                 // empty check box glyphs
    G_FONT = FONT_A;
}

static void erase_marker(int x2, int y, int row8, int flag) {
    u8 save = G_COLOR;
    G_COLOR = G_BGCOLOR;
    double top = (double)row8 * 1.75 + ((double)y * 1.75 + 15.0);
    double lim = (double)row8 * 1.75 + ((double)y * 1.75 + 19.0);
    for (int c = (int)top; (double)c < lim; c++) gfx_hline(x2 - 16, x2 - 8, c, flag);
    G_COLOR = save;
}

// 1000:1044 - move the highlight between options; Enter ticks the box and returns the option number.
int menu_select(int x2, int y) {
    int sel = 1;
    if (UI_CONTEXT != 0) sel = MENU_LAST(UI_CONTEXT);
    G_FONT = FONT_B;
    gfx_moveto(x2 - 20, (int)(((double)y * 1.75 + 13.0) + 14.0 * (double)MENU_LINE(sel)));
    int row = MENU_LINE(sel);
    gfx_text(S(0x120a));                 // filled marker glyphs
    for (;;) {
        row <<= 3;
        int key = wait_key();
        if (key == 0x1c0d) {             // Enter
            erase_marker(x2, y, row, 0xff);
            G_FONT = FONT_A;
            if (UI_CONTEXT != 0) {
                MENU_LAST(UI_CONTEXT) = (i16)sel;
                if (MENU_LINES - 1 == MENU_LINE(sel)) MENU_LAST(UI_CONTEXT) = 1;
            }
            for (int c = x2 - 16; c < x2 - 8; c++) {        // draw the tick, one pixel column at a time
                gfx_plot(c, (int)((double)W(0x11f0 + (c - x2 + 16) * 2) + ((double)row * 1.75 + (double)y * 1.75)));
                host_delay_ms(12);
            }
            return sel;
        }
        if (key == 0x1245) game_longjmp(999);               // 'E': abandon to the main loop
        if (key == 0x1f53 && UI_CONTEXT == 5) break;        // 'S': save game
        if (key == 0x3042 && UI_CONTEXT == 1) show_status_chart();   // 'B': bar chart
        if (key == 0x4f00) while (MENU_LINE(sel) < MENU_LINES - 1) sel++;   // End
        if (key == 0x4700) sel = 1;                                           // Home
        if (key == 0x4800 && sel > 1) sel--;                                  // Up
        if (key == 0x5000 && MENU_LINES - 1 != MENU_LINE(sel)) sel++;         // Down
        {
            u8 save = G_COLOR;
            G_COLOR = G_BGCOLOR;
            double top = (double)row * 1.75 + ((double)y * 1.75 + 15.0);
            double lim = (double)row * 1.75 + ((double)y * 1.75 + 19.0);
            for (int c = (int)top; (double)c < lim; c++) gfx_hline(x2 - 16, x2 - 8, c, 1);
            gfx_moveto(x2 - 20, (int)(((double)y * 1.75 + 13.0) + 14.0 * (double)MENU_LINE(sel)));
            G_COLOR = save;
        }
        gfx_text(S(0x120d));
        row = MENU_LINE(sel);
    }
    save_game();
    return 1;
}
