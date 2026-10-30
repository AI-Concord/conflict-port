#include <string.h>
#include "ega.h"

uint8_t ega_plane[4][0x10000];
uint8_t ega_latch[4];
uint8_t ega_gc[16];
uint8_t ega_seq[8];
uint8_t ega_palette[16] = {0, 1, 2, 3, 4, 5, 0x14, 7, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f};
int ega_mode = 3;

void ega_set_mode(int mode) {
    ega_mode = mode;
    memset(ega_plane, 0, sizeof ega_plane);
    memset(ega_gc, 0, sizeof ega_gc);
    memset(ega_seq, 0, sizeof ega_seq);
    ega_gc[8] = 0xff; ega_seq[2] = 0x0f; ega_gc[7] = 0x0f;
}

uint8_t ega_read(uint32_t off) {
    off &= 0xffff;
    for (int p = 0; p < 4; p++) ega_latch[p] = ega_plane[p][off];
    if (ega_gc[5] & 8) {                    // read mode 1: colour compare
        uint8_t r = 0xff;
        for (int p = 0; p < 4; p++) if (ega_gc[7] & (1 << p)) {
            uint8_t cmp = (ega_gc[2] & (1 << p)) ? 0xff : 0x00;
            r &= (uint8_t)~(ega_latch[p] ^ cmp);
        }
        return r;
    }
    return ega_latch[ega_gc[4] & 3];
}

static uint8_t rotr8(uint8_t v, int n) { n &= 7; return (uint8_t)((v >> n) | (v << (8 - n))); }
static uint8_t logic(uint8_t v, uint8_t l) {
    switch ((ega_gc[3] >> 3) & 3) { case 0: return v; case 1: return v & l; case 2: return v | l; default: return v ^ l; }
}

void ega_write(uint32_t off, uint8_t val) {
    uint8_t mask = ega_gc[8], out[4]; int wm = ega_gc[5] & 3;
    off &= 0xffff;
    if (wm == 0) {
        uint8_t v = rotr8(val, ega_gc[3] & 7);
        for (int p = 0; p < 4; p++) {
            uint8_t d = v;
            if (ega_gc[1] & (1 << p)) d = (ega_gc[0] & (1 << p)) ? 0xff : 0x00;
            d = logic(d, ega_latch[p]);
            out[p] = (d & mask) | (ega_latch[p] & ~mask);
        }
    } else if (wm == 1) {
        for (int p = 0; p < 4; p++) out[p] = ega_latch[p];
    } else if (wm == 2) {
        for (int p = 0; p < 4; p++) {
            uint8_t d = (val & (1 << p)) ? 0xff : 0x00;
            d = logic(d, ega_latch[p]);
            out[p] = (d & mask) | (ega_latch[p] & ~mask);
        }
    } else {
        uint8_t v = rotr8(val, ega_gc[3] & 7), m = mask & v;
        for (int p = 0; p < 4; p++) {
            uint8_t d = (ega_gc[0] & (1 << p)) ? 0xff : 0x00;
            out[p] = (d & m) | (ega_latch[p] & ~m);
        }
    }
    for (int p = 0; p < 4; p++) if (ega_seq[2] & (1 << p)) ega_plane[p][off] = out[p];
}

void ega_to_bgra(uint32_t *out) {
    uint32_t pal[16];
    for (int c = 0; c < 16; c++) {
        uint8_t e = ega_palette[c];
        uint32_t r = ((e >> 2) & 1) * 0xaa + ((e >> 5) & 1) * 0x55;
        uint32_t g = ((e >> 1) & 1) * 0xaa + ((e >> 4) & 1) * 0x55;
        uint32_t b = ((e >> 0) & 1) * 0xaa + ((e >> 3) & 1) * 0x55;
        pal[c] = 0xff000000u | (r << 16) | (g << 8) | b;
    }
    if (ega_mode != 0x10) { for (int i = 0; i < 640 * 350; i++) out[i] = 0xff000000u; return; }
    for (int y = 0; y < 350; y++) for (int xb = 0; xb < 80; xb++) {
        uint32_t off = (uint32_t)(y * 80 + xb);
        uint8_t p0 = ega_plane[0][off], p1 = ega_plane[1][off], p2 = ega_plane[2][off], p3 = ega_plane[3][off];
        for (int bit = 7; bit >= 0; bit--)
            *out++ = pal[((p0 >> bit) & 1) | (((p1 >> bit) & 1) << 1) | (((p2 >> bit) & 1) << 2) | (((p3 >> bit) & 1) << 3)];
    }
}
