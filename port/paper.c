// Newspaper front pages, title/intro screens, saved games and new-game setup
// (original code 1000:14BC - 1000:3137, plus the icon helper at 1000:B8AB).
#include <stdlib.h>
#include "conflict.h"

#define FONT_A UW(0xb294)
#define FONT_B UW(0xb296)
#define NEWS_COUNT   W(0xb25c)                    // headlines this month
#define HEADLINE(i)  UW(0xa16e + (i) * 2)         // near pointers to headline text
#define SY(v) ((double)(v) * 1.75)

static void paper_side_story(int x, int yv) {     // 1000:1D09 - boxed secondary headline
    int v = rnd(9);
    int left = (rnd(3) == 1) ? 0x113 : 0xbe;
    G_COLOR = 0xf;
    gfx_fillrect(x + left, x + 0x1bd, (int)SY(yv + 0x34), (int)SY(yv + 0x46));
    G_COLOR = 0;
    int ys = (int)SY(yv + 0x37);
    gfx_hline(x + left - 6, x + 0x1b9, ys, 0);
    gfx_vline(ys, (int)SY(yv + 0x5a), x + left - 6, 0);
    gfx_moveto(x + left + 2, (int)SY(yv + 0x3c));
    gfx_text(S(UW((left == 0x113 ? 0x1364 : 0x1474) + (v - 1) * 2)));
}

static void paper_briefly(int x, int yv) {        // 1000:1F6F - "BRIEFLY" side column
    int ys = (int)SY(yv + 0x16);
    gfx_hline(x + 0xf, x + 100, ys, 0);
    int yb = (int)SY(yv + 0x5a);
    gfx_vline(ys, yb, x + 0xf, 0);
    gfx_vline(ys, yb, x + 100, 0);
    gfx_moveto(x + 0x1c, (int)SY(yv + 0x17));
    G_FONT = FONT_B;
    gfx_text(S(0x14ac));
    G_FONT = FONT_A;
}

void draw_newspapers(void) {                      // 1000:14BC - stack of up to five front pages
    G_COLOR = 7;
    gfx_fillrect(0, 0x27f, 0, 0x15d);
    G_COLOR = 0;
    int first = NEWS_COUNT > 5 ? NEWS_COUNT - 5 : 0;
    for (int i = 0; NEWS_COUNT - first > i; i++) {
        int h = 0x69 - rnd(15);
        int front = (NEWS_COUNT - 1 - first == i);
        if (front) h = 0x5a;
        int style = rnd(9) - 1;
        int yv = i * 28, x = i * 16;
        double dy = SY(yv);
        int cy = 0;
        gfx_hline(x + 5, x + 0x1c2, (int)(dy + 5.0), 0);
        G_COLOR = 0xf;
        double d = SY(yv + h) - 1.0;
        if (d < 348.0) gfx_fillrect(x + 6, x + 0x1c1, (int)(dy + 6.0), (int)d);
        else gfx_fillrect(x + 6, x + 0x1c1, (int)(dy + 6.0), 0x15c);
        G_COLOR = 0;
        gfx_hline(x + 0xf, x + 0x1b8, (int)(dy + 30.0), 0);
        if (yv + h < 0xc4) {
            int yb = (int)SY(yv + h);
            gfx_hline(x + 5, x + 21, yb, 0);
            int yt = (int)(dy + 5.0);
            gfx_vline(yt, yb, x + 5, 0);
            int yb2 = (int)(SY(yv + 28) + 5.0);
            gfx_vline(yt, yb2, x + 0x1c2, 0);
            gfx_vline((int)(dy + 6.0), yb2, x + 0x1c3, 0);
        } else {
            int yt = (int)(dy + 5.0);
            gfx_vline(yt, 0x15a, x + 5, 0);
            gfx_vline(yt, 0x15a, x + 0x1c2, 0);
            gfx_vline((int)(dy + 6.0), 0x15a, x + 0x1c3, 0);
        }
        if (front) {
            if (i == 4) {
                gfx_vline((int)(dy + 5.0), 0x15a, x + 0x1c2, 0);
                gfx_vline((int)(dy + 6.0), 0x15a, x + 0x1c3, 0);
            } else {
                int yb = (int)SY(yv + h);
                gfx_hline(x + 5, x + 0x1c2, yb, 0);
                gfx_vline((int)(dy + 5.0), yb, x + 0x1c2, 0);
                gfx_vline((int)(dy + 6.0), yb, x + 0x1c3, 0);
            }
            G_FONT = FONT_B;                        // columns of "body text" made of filler glyphs
            for (int cx = 0; cx < 0x1a9; cx += 0x55) {
                for (cy = 0; cy < 0x36; cy += 0xb) {
                    double t = SY(yv + cy);
                    if (!(t + 53.0 < 339.5)) break;
                    int tx = x + 0x14 + cx;
                    gfx_moveto(tx, (int)(t + 54.0)); gfx_text(S(0x1498));
                    gfx_moveto(tx, (int)(t + 63.0)); gfx_text(S(0x14a2));
                }
            }
            G_FONT = FONT_A;
            if (yv + cy + 0x2a < 0xc2) paper_side_story(x, yv);
        }
        gfx_moveto(x + (0x68 - W(0x1486 + style * 2)), (int)(dy + 10.0));
        if (style == 0 || style == 3) G_BIGFONT = (u16)(UW(0xb298) - 0x900);
        else G_BIGFONT = (u16)(UW(0xb29a) - 0x900);
        gfx_text_big(S(UW(0x12a0 + style * 2)));    // masthead
        int ty = (int)(dy + 41.0);
        const char *head = S(HEADLINE(i + first));
        gfx_moveto(x + ((0x1bd - gfx_text_width(head)) >> 1) + 5, ty);
        if (front && gfx_text_width(head) < 0x154) {
            paper_briefly(x, yv);
            gfx_moveto(x + ((0x168 - gfx_text_width(head)) >> 1) + 0x5a, ty);
        }
        gfx_text(S(HEADLINE(i + first)));
    }
}

void draw_unit_icon(const u8 *icon) {             // 1000:B8AB - 32x32 icon, coloured by weapon type
    u8 idx = (u8)(((i16)(OFF(icon) - UW(0xb29e))) / 0x80);
    u8 fg = 0;
    if (idx == 8 || idx == 10 || idx == 13) fg = 4;
    else if (idx == 9 || idx == 11 || idx == 12) fg = 1;
    u8 sc = G_COLOR, sb = G_BGCOLOR;
    G_COLOR = fg; G_BGCOLOR = 0xf;
    gfx_draw_icon(icon);
    G_COLOR = sc; G_BGCOLOR = sb;
}

void intro_screen(void) {                         // 1000:2058 - video init and one of four opening texts
    gfx_init();
    load_fonts();
    G_COLOR = 4;
    gfx_fillrect(0, 0x27f, 0, 0x15d);
    G_BGCOLOR = 0xf;
    G_COLOR = 0;
    char *title = S(0xa1f2), *body = S(0xa2ba), *extra = S(0xa3b4);
    strcpy(body, S(0x14b4));
    int which = rnd(4);
    if (which == 1) {
        static const u16 parts[] = {0x14e7, 0x14ff, 0x151f, 0x1534, 0x154d, 0x156b, 0x157e};
        strcpy(title, S(0x14b5));
        strcpy(extra, S(0x14c9));
        for (int i = 0; i < 7; i++) strcat(extra, S(parts[i]));
        text_box(0xa5, 0x28, 0x1db, 0xa0, title, body, extra);
    }
    if (which == 2) {
        static const u16 parts[] = {0x15a6, 0x15ce, 0x15f4, 0x1621, 0x164a, 0x1674, 0x169e, 0x16c6};
        strcpy(title, S(0x1598));
        for (int i = 0; i < 8; i++) strcat(extra, S(parts[i]));
        text_box(0x78, 0x28, 0x208, 0xa0, title, body, extra);
    }
    if (which == 3) {
        static const u16 parts[] = {0x16fe, 0x172c, 0x175b};
        strcpy(title, S(0x16f2));
        for (int i = 0; i < 3; i++) strcat(extra, S(parts[i]));
        text_box(0x66, 0x46, 0x21a, 0x84, title, body, extra);
    }
    if (which == 4) {
        static const u16 parts[] = {0x179a, 0x17bd, 0x17e0, 0x180c, 0x1834, 0x185d};
        strcpy(title, S(0x178e));
        for (int i = 0; i < 6; i++) strcat(extra, S(parts[i]));
        text_box(0x78, 0x32, 0x208, 0x98, title, body, extra);
    }
}

void title_screen(void) {                         // 1000:2337 - logo, "CONFLICT / Load Game", copyright
    char copyright[100];
    strcpy(copyright, S(0x1881));
    W(0xb278) = 0;
    draw_box(0x94, 0x32, 0x1e0, 0x50);
    gfx_moveto(0xb9, 0x62);
    G_BIGFONT = (u16)(UW(0xb298) - 0x900);
    gfx_text_big(S(0x18a4));
    gfx_moveto(0xaa, 0x7c);
    gfx_text(S(0x18b4));
    if (menu(0xfa, 0x66, 0x17c, 0x93, S(UW(0xcf8)), 0) == 2) W(0xb278) = 1;
    draw_box(0xe, 0xa0, 0x265, 0xb4);
    gfx_moveto((0x254 - gfx_text_width(copyright)) / 2 + 0x11, 0x126);
    gfx_text(copyright);
    for (long n = 0; n < 20000; n++) if (gfx_getkey() & 0xff) break;
}

void thinking_start(void) {                       // 1000:245C
    W(0xb2c4) = (i16)(gfx_text_width(S(0x18d8)) + 0x68);
    draw_box(100, 0xaa, 0xe6, 0xb6);
    gfx_moveto(0x69, 0x12e);
    gfx_text(S(0x18e4));
}

void thinking_dot(void) {                         // 1000:24A1
    gfx_moveto(W(0xb2c4), 0x12e);
    gfx_text(S(0x18f0));
    W(0xb2c4) += 10;
}

void icon_wallpaper(int kind) {                   // 1000:24C2 - tile the screen with unit icons (99 = random mix)
    int tiles[120], prev = 0;                     // the original left 'prev' uninitialised
    for (int i = 0; i < 0x78; i++) {
        if (kind == 99) {
            int v;
            do v = rnd(6) + 6; while (v == prev);
            tiles[i] = v; prev = v;
        } else tiles[i] = kind;
    }
    int n = 0;
    for (int row = 6; row < 0xbe; row += 0x19)
        for (int col = 0; col < 0xf; col++) {
            gfx_moveto(col * 0x28 + 0x14, (int)SY(row));
            draw_unit_icon((const u8 *)S((u16)(tiles[n] * 0x80 + UW(0xb29e))));
            n++;
        }
}

// The saved game is a straight dump of the game-state globals, in this order.
static const struct { u16 off, size, count; } save_layout[] = {
    {0x92f0, 2, 8}, {0x9300, 2, 8}, {0x9310, 2, 0x40}, {0x9390, 2, 8}, {0x93a0, 2, 0x280}, {0x98a0, 2, 8}, {0x98b0, 2, 9},
    {0x98c2, 2, 8}, {0x98d2, 2, 0x3b6}, {0xa03e, 2, 8}, {0xa04e, 2, 8}, {0xa05e, 2, 0x40}, {0xa0de, 2, 8}, {0xa0ee, 2, 8},
    {0xa0fe, 2, 8}, {0xa10e, 2, 8}, {0xa11e, 2, 8}, {0xa12e, 2, 8}, {0xa13e, 2, 8}, {0xa14e, 2, 8}, {0xa15e, 2, 8},
    {0xa16e, 2, 0x19}, {0xa1a0, 2, 0x19}, {0xa1d2, 2, 8}, {0xa1e2, 2, 8}, {0xa4ae, 1, 0x9c4}, {0xae72, 1, 1000},
    {0xb25a, 2, 1}, {0xb25c, 2, 1}, {0xb25e, 2, 1}, {0xb260, 2, 1}, {0xb262, 2, 1}, {0xb264, 2, 1}, {0xb266, 2, 1}, {0xb268, 2, 1},
    {0xb26a, 2, 1}, {0xb26c, 2, 1}, {0xb26e, 2, 1}, {0xb270, 2, 1}, {0xb272, 2, 1}, {0xb274, 2, 1}, {0xb276, 2, 1},
};
#define SAVE_ITEMS (sizeof save_layout / sizeof save_layout[0])

void save_game(void) {                            // 1000:25B3
    W(0xb270) = 0;
    G_FONT = FONT_A;
    int slot = menu(100, 5, 0x15e, 0x82, S(UW(0xcf4)), 0);
    if (slot == 6) return;
    char name[14]; name[0] = 0;
    if (slot >= 1 && slot <= 5) strcpy(name, S((u16)(0x18f2 + (slot - 1) * 10)));
    FILE *f = host_fopen(name, "wb");
    if (!f) return;
    for (unsigned i = 0; i < SAVE_ITEMS; i++) fwrite(DSP + save_layout[i].off, save_layout[i].size, save_layout[i].count, f);
    fclose(f);
}

void load_game(void) {                            // 1000:29ED
    W(0xb278) = 0;
    int slot = menu(0xb4, 0x1e, 0x1c2, 0xa0, S(0x1927), 0);
    if (slot == 6) return;
    char name[14]; name[0] = 0;
    if (slot >= 1 && slot <= 5) strcpy(name, S((u16)(0x195e + (slot - 1) * 10)));
    FILE *f = host_fopen(name, "rb");
    if (!f) { game_longjmp(999); return; }
    W(0xb278) = 1;
    for (unsigned i = 0; i < SAVE_ITEMS; i++) fread(DSP + save_layout[i].off, save_layout[i].size, save_layout[i].count, f);
    fclose(f);
}

void new_game_init(void) {                        // 1000:2E39
    int prev = 0;
    W(0xb268) = 1;
    for (int i = 0; i < 0x19; i++) { W(0xa16e + i * 2) = 0; W(0xa1a0 + i * 2) = 0; }
    W(0xb25c) = 0; W(0xb25a) = 0; W(0xb260) = 0; W(0xb25e) = 0; W(0xb26a) = 0; W(0xb270) = 0;
    W(0xb266) = 3; W(0xb262) = 0x96; W(0xb26c) = 0; W(0xb276) = 0; W(0xb26e) = 7; W(0xb264) = 0;
    add_headline(UW(0x19f6 + (rnd(2) - 1) * 2), 7, 7);
    add_headline(UW(0x19f6 + (rnd(2) + 1) * 2), 7, 7);
    for (int a = 1; a < 8; a++)
        for (int b = 0; b < a; b++) {
            int v;
            do v = rnd(4) + 3; while (v == prev);
            W(0xa05e + a * 0x10 + b * 2) = (i16)v;       // relations between countries a and b
            W(0x9310 + a * 0x10 + b * 2) = 0;
            prev = v;
        }
    for (int a = 0; a < 8; a++) for (int b = 0; b < 8; b++) for (int c = 0; c < 9; c++) W(0x93a0 + a * 0xa0 + b * 0x14 + c * 2) = 0;
    init_countries();
    for (int i = 0; i < 8; i++) {
        W(0x98a0 + i * 2) = 0; W(0x98b0 + i * 2) = 0; W(0xa15e + i * 2) = 0;
        W(0x93aa + i * 0xb4) = 0x19; W(0x93ae + i * 0xb4) = 5;
        W(0x9390 + i * 2) = 0; W(0xa1e2 + i * 2) = 0; W(0xa1d2 + i * 2) = 0;
    }
    W(0x98be) = 7; W(0x98c0) = 0;
}

void index_news_by_country(void) {                // 1000:3036 - gather this month's stories per country
    if (W(0xb25a) != 0) for (int i = 0; i < 0x9c4; i++) B(0x87ca + i) = 0;
    int n = 0;
    for (int c = 0; c < 8; c++) {
        UW(0xa1e2 + c * 2) = (u16)(n + 0x87ca);
        for (int a = 0; a < W(0xb25a); a++) {
            if (contains4(S(UW(0xa1a0 + a * 2)), S(UW(0xd30 + c * 2)))) {
                const char *p = S(UW(0xa1a0 + a * 2));
                if (contains4(p, S(0x19fe))) UW(0xa1e2 + c * 2) = (u16)(n + 0x87ca);
                while (*p && n < 0x9c4 - 12) { B(0x87ca + n) = (u8)*p++; n++; }   // bounded; the original was not
                B(0x87ca + n) = 0x20; n++;
            }
        }
        B(0x87ca + n) = 0; n++;
    }
}
