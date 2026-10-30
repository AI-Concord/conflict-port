// Graphics library: a line-for-line translation of the game's EGA assembly
// module onto the virtual adapter in ega.c. Register names are kept where the
// original relies on exact byte/bit arithmetic.
#include "conflict.h"
#include <stdlib.h>
#include "ega.h"

#define CUR_OFF   UW(0x8370)      // byte offset of the text/drawing cursor in video memory
#define CUR_BIT   B(0x8372)       // bit offset within that byte
#define CUR_MASK  B(0x8373)
static const u8 lmask[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};
static const u8 rmask[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

#define VIS  0u                   // visible page starts at A000:0000
#define PAGE 0x6d60u              // off-screen page (segment A6D6)
#define VR(base, off)     ega_read((base) + (u16)(off))
#define VW(base, off, v)  ega_write((base) + (u16)(off), (v))

static u8 *pictures[32]; static int picture_len[32];
static u8 *maps[16];

static void gc_reset(void) {                       // 1F39:034A
    ega_gcw(0xff08); ega_gcw(0x0007); ega_gcw(0x0005); ega_gcw(0x0003);
}

// 1F39:0302 - returns the byte offset; sets cursor. *bit receives x & 7.
static u16 setpos(int x, int y, u8 *mask_out) {
    u16 di = (u16)((u16)y * 80 + ((u16)x >> 3));
    u8 cl = (u8)x & 7;
    CUR_OFF = di; CUR_BIT = cl;
    u8 al = (u8)((0x7f >> cl) | (0x7f << (8 - cl)));
    CUR_MASK = al;
    if (mask_out) *mask_out = al;
    return di;
}

// Shared tail of the horizontal span routines: fill from x1 to x2 on one row.
static void span(u32 base, u16 di, u16 x1, u16 x2) {
    u8 bh = lmask[x1 & 7], bl = rmask[x2 & 7];
    u16 cx = (u16)((x2 >> 3) - (x1 >> 3));
    u16 si = di;
    if (!(bh & 0x80)) {
        if (cx == 0) { bl &= bh; goto last; }
        ega_gcw((u16)(bh << 8) | 8); VW(base, di, VR(base, si)); si++; di++;
        cx--;
    }
    ega_gcw(0xff08);
    while (cx) { VW(base, di, VR(base, si)); si++; di++; cx--; }
last:
    ega_gcw((u16)(bl << 8) | 8); VW(base, di, VR(base, si));
}

static void hline_raw(int x1, int x2, int y) {     // 1F39:035E
    UW(0x838c) = (u16)x1; UW(0x838e) = (u16)x2; UW(0x8390) = (u16)y;
    u16 di = setpos(x1, y, NULL);
    ega_gcw((u16)(G_COLOR << 8) | 0); ega_gcw(0x0f01); ega_gcw(0x0003);
    span(VIS, di, (u16)x1, (u16)x2);
    ega_gcw(0x0000); ega_gcw(0x0001);
    gc_reset();
}

static void hline_page(int x1, int x2, int y) {    // 1F39:03D7 (off-screen page, y measured from the bottom)
    u16 cx = (u16)(0x15d - (u16)y);
    UW(0x838c) = (u16)x1; UW(0x838e) = (u16)x2; UW(0x8390) = cx;
    u16 di = (u16)(cx * 80 + ((u16)x1 >> 3));
    span(PAGE, di, (u16)x1, (u16)x2);
}

static void vline_raw(int y1, int y2, int x) {     // 1F39:044F
    UW(0x8390) = (u16)y1; UW(0x8392) = (u16)y2; UW(0x838c) = (u16)x;
    u16 cx = (u16)(y2 - y1 + 1);
    u8 al; u16 di = setpos(x, y1, &al);
    al = (u8)~al;
    ega_gcw((u16)(al << 8) | 8);
    ega_gcw((u16)(G_COLOR << 8) | 0); ega_gcw(0x0f01); ega_gcw(0x0003);
    do { VW(VIS, di, VR(VIS, di) | al); di += 80; } while (--cx);
    ega_gcw(0x0000); ega_gcw(0x0001);
    gc_reset();
}

static u8 map_char(u8 c) {                         // 1F39:0659
    if (c >= 0x30) return c;
    switch (c) { case ' ': return 0x40; case '.': return 0x60; case '-': return 0x5c; case '!': return 0x5e; case '%': return 0x5b; case '$': return 0x5d; }
    return 0x5f;
}

static void glyph8(const u8 *g) {                  // 1F39:048E
    u8 cl = (u8)(-CUR_BIT) & 7;
    UW(0x8394) = (u16)(((u8)(0xff << cl) << 8) | cl);
    u16 di = CUR_OFF; u8 bl = G_COLOR;
    ega_gcw(0x0a05); ega_gcw(0x0003); ega_gcw(0x0007);
    for (int row = 0; row < 8; row++) {
        if (cl == 0) {
            ega_gcw((u16)(g[row] << 8) | 8); VW(VIS, di, VR(VIS, di) & bl); di += 80;
        } else {
            u16 ax = (u16)(g[row] << cl);
            ega_gcw((u16)(ax & 0xff00) | 8); VW(VIS, di, VR(VIS, di) & bl);
            ega_gcw((u16)((ax & 0xff) << 8) | 8); di++; VW(VIS, di, VR(VIS, di) & bl);
            di += 0x4f;
        }
    }
    gc_reset(); ega_gcw(0x0f07);
}

static void glyph16(const u8 *g) {                 // 1F39:04FC
    u8 cl = (u8)(-CUR_BIT) & 7;
    UW(0x8394) = (u16)(0x0800 | cl);
    u16 di = CUR_OFF; u8 bl = G_COLOR;
    ega_gcw(0x0a05); ega_gcw(0x0003); ega_gcw(0x0007);
    for (int row = 0; row < 16; row++) {
        u8 b0 = g[row * 2], b1 = g[row * 2 + 1];
        if (cl == 0) {
            ega_gcw((u16)(b0 << 8) | 8); VW(VIS, di, VR(VIS, di) & bl); di++;
            ega_gcw((u16)(b1 << 8) | 8); VW(VIS, di, VR(VIS, di) & bl); di += 0x4f;
        } else {
            u16 a0 = (u16)(b0 << cl), a1 = (u16)(b1 << cl);
            ega_gcw((u16)(a0 & 0xff00) | 8); VW(VIS, di, VR(VIS, di) & bl); di++;
            ega_gcw((u16)((((a1 >> 8) | (a0 & 0xff)) & 0xff) << 8) | 8); VW(VIS, di, VR(VIS, di) & bl); di++;
            ega_gcw((u16)((a1 & 0xff) << 8) | 8); VW(VIS, di, VR(VIS, di) & bl);
            di += 0x4e;
        }
    }
    gc_reset(); ega_gcw(0x0f07);
}

static void advance(u8 al) {                       // common cursor advance after a glyph
    al = (u8)(al + CUR_BIT);
    CUR_OFF = (u16)(CUR_OFF + (al >> 3));
    CUR_BIT = al & 7;
}

void gfx_text(const char *s) {                     // 1F39:08E4 / 0584
    const u8 *font = (const u8 *)S(G_FONT);
    for (const u8 *p = (const u8 *)s; *p; p++) {
        const u8 *g = font + map_char(*p) * 9;
        glyph8(g);
        advance(g[8]);
    }
}

void gfx_text_big(const char *s) {                 // 1F39:08F8 / 05F2
    const u8 *font = (const u8 *)S(G_BIGFONT);
    for (const u8 *p = (const u8 *)s; *p; p++) {
        const u8 *g = font + map_char(*p) * 36;
        glyph16(g);
        advance(g[32] >> 3);
    }
}

int gfx_text_width(const char *s) {                // 1F39:09E9
    const u8 *font = (const u8 *)S(G_FONT);
    const u8 *si = (const u8 *)s; u16 dx = 0;
    for (;;) {
        u8 al = map_char(*si++);
        const u8 *g = font + al * 9;
        if (*si == 0) {
            // Last character: the original adds 8 only if some row uses the rightmost pixel column.
            u8 bh = 0; for (int i = 0; i < 8; i++) bh |= g[i];
            dx += (bh & 1) ? 8 : 0;
            return (i16)dx;
        }
        dx += g[8];
    }
}

int gfx_moveto(int x, int y) { return setpos(x, y, NULL); }          // 1F39:0801

int gfx_plot(int x, int y) {                                          // 1F39:0813 / 032C
    u8 al; u16 di = setpos(x, y, &al);
    al = (u8)~al;
    ega_gcw((u16)(al << 8) | 8); ega_gcw(0x0205); ega_gcw(0x0003);
    (void)VR(VIS, di); VW(VIS, di, G_COLOR);
    gc_reset();
    return di;
}

void gfx_hline(int x1, int x2, int y, int use_color) {               // 1F39:0827
    if (use_color == 0) { u8 save = G_COLOR; G_COLOR = 0; hline_raw(x1, x2, y); G_COLOR = save; }
    else hline_raw(x1, x2, y);
}

void gfx_vline(int y1, int y2, int x, int use_color) {               // 1F39:0858
    if (use_color == 0) { u8 save = G_COLOR; G_COLOR = 0; vline_raw(y1, y2, x); G_COLOR = save; }
    else vline_raw(y1, y2, x);
}

void gfx_fillrect(int x1, int x2, int y1, int y2) {                  // 1F39:091E / 05E0
    u16 cx = (u16)y1, dx = (u16)(y2 + 1);
    do { hline_raw(x1, x2, cx); cx++; } while (cx != dx);
}

void gfx_draw_icon(const u8 *icon) {                                 // 1F39:07DF / 028E (32x32, two colours)
    u16 di = CUR_OFF; u8 bl = G_COLOR, bh = G_BGCOLOR;
    ega_gcw(0x0a05); ega_gcw(0x0003); ega_gcw(0x0007);
    for (int row = 0; row < 32; row++) {
        for (int i = 0; i < 4; i++) {
            u8 m = *icon++;
            ega_gcw((u16)(m << 8) | 8); VW(VIS, di, VR(VIS, di) & bl);
            ega_gcw((u16)((u8)~m << 8) | 8); VW(VIS, di, VR(VIS, di) & bh);
            if (i < 3) di++;
        }
        di += 0x4d;
    }
    gc_reset(); ega_gcw(0x0f07);
}

void gfx_draw_picture(int n) {                                       // 1F39:0746 / 0256 (160x200 black and white)
    const u8 *si = pictures[n];
    if (!si) return;
    u16 di = CUR_OFF;
    gc_reset(); ega_gcw(0x0805); ega_gcw(0x0001);
    for (int row = 0; row < 200; row++) {
        for (int i = 0; i < 20; i++) { VW(VIS, di, VR(VIS, di) & *si++); di++; }
        di += 0x3c;
    }
    gc_reset();
}

void gfx_draw_map(int n) {                                           // 1F39:0937
    const u8 *si = maps[n];
    ega_gcw(0x0f01); ega_gcw(0x0003);
    UW(0x839c) = 0; UW(0x839a) = 0;
    for (;;) {
        ega_gcw((u16)(*si << 8) | 0); si++;
        UW(0x8396) = (u16)(si[0] | (si[1] << 8)); si += 2;
        UW(0x8398) = (u16)(si[0] | (si[1] << 8)); si += 2;
        for (;;) {
            u16 y = UW(0x839c);
            if (y > 0x15d) goto done;
            if (y == UW(0x8398)) break;
            hline_page(UW(0x839a), 0x27f, y);
            UW(0x839c)++; UW(0x839a) = 0;
        }
        { u16 x1 = UW(0x839a), bx = UW(0x8396); UW(0x839a) = bx; hline_page(x1, (u16)(bx - 1), UW(0x839c)); }
        if (UW(0x8398) == 0x15d && UW(0x8396) == 0x27f) break;
    }
done:
    gc_reset(); ega_gcw(0x0105);
    for (u16 i = 0; i < 0x6d60; i++) VW(VIS, i, VR(PAGE, i));
    ega_gcw(0x0000); ega_gcw(0x0001);
    gc_reset();
}

void gfx_recolor(int x, int y, int w, int h, int from, int to) {     // 1F39:0889
    u16 si = setpos(x, y, NULL);
    u16 cx = (u16)w >> 3; u16 rows = (u16)h;
    ega_gcw(0x0a05); ega_gcw(0x0f07); ega_gcw((u16)((u8)from << 8) | 2);
    do {
        u16 s = si, d = si, c = cx;
        do { u8 m = VR(VIS, s); s++; ega_gcw((u16)(m << 8) | 8); VW(VIS, d, (u8)to); d++; } while (--c);
        si += 0x50;
    } while (--rows);
    gc_reset();
}

void gfx_copy_from_page(int x1, int x2, int y1, int y2) {            // 1F39:06DD
    ega_gcw(0x0105);
    x1--; x2 += 3; y1--; y2 += 3;
    u16 di = setpos(x1, y1, NULL), si = di;
    u16 rows = (u16)(y2 - y1 + 1);
    u16 bx = ((u16)x1 & 7) ? 1 : 0;
    u16 n = (u16)((((u16)(x2 - x1 - 1)) >> 3) + 1 + bx);
    u16 skip = (u16)(0x50 - n);
    do {
        for (u16 c = n; c; c--) { VW(VIS, di, VR(PAGE, si)); si++; di++; }
        si += skip; di += skip;
    } while (--rows);
    gc_reset();
}

// 1F39:0A58 - poll the keyboard. The original has a slip here: an ordinary key is
// swallowed and reported as "no key", and an extended key waits for a second key press.
// As a result "press any key" pauses could only be cut short with a cursor key and the
// E (abandon) key never worked during the month-end calculations. The front end can set
// gfx_plain_keys to get the evidently intended behaviour instead.
int gfx_plain_keys = 0;
int gfx_getkey(void) {
    if (!host_key_available()) return 0;
    u16 k = host_key_read();
    if (gfx_plain_keys) return (k & 0xff) ? (k & 0xff) : (k >> 8);
    if (k & 0xff) return 0;
    k = host_key_read();
    return k >> 8;
}

static int load_list(u16 names, u8 **table, int *lens) {
    const char *si = S(names); int i = 0;
    while (*si) {
        FILE *f = host_fopen(si, "rb");
        if (!f) return -2;
        static u8 buf[40000]; size_t got = fread(buf, 1, sizeof buf, f); fclose(f);
        table[i] = (u8 *)malloc(got ? got : 1); memcpy(table[i], buf, got);
        if (lens) lens[i] = (int)got;
        i++;
        si += strlen(si) + 1;
    }
    return 0;
}
int gfx_load_pictures(void) { return load_list(0x868e, pictures, picture_len); }   // 1F39:0783
int gfx_load_maps(void) { return load_list(0x865e, maps, NULL); }                   // 1F39:078B

int gfx_init(void) {                                                 // 1F39:0771
    for (int i = 0; i < 350; i++) UW(0x83a2 + i * 2) = (u16)(i * 80);
    host_set_video_mode(0x10);
    return 0;
}
