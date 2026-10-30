// Map overlays: fallen governments, wars and nuclear sites shown on the regional maps
// (original 1000:3D5E - 1000:4674).
#include "conflict.h"

#define ICONS       UW(0xb29e)
#define TURN        W(0xb268)
#define WAR(a, b)   W(0x9310 + (a) * 0x10 + (b) * 2)
#define ICON(n)     ((const u8 *)S((u16)(ICONS + (n) * 0x80)))

static double delay_factor(void) { float f; memcpy(&f, DSP + 0xb27a, 4); return (double)f; }
// The original scaled busy-wait loops by a factor measured at start-up.
void delay_loop(double base) { host_delay_ms((int)(base * 0.034)); }
int delay_or_key(double base) {
    long n = (long)(base / delay_factor());
    for (long i = 0; i < n; i++) if (gfx_getkey() & 0xff) return 1;
    return 0;
}

static void flash_icon(int x, int y, int icon) {  // 1000:4384 / 1000:44D9 - blink an icon five times
    gfx_moveto(x, (int)((double)y * 1.75));
    draw_unit_icon(ICON(icon));
    for (int i = 0; i < 5; i++) {
        gfx_moveto(x, (int)((double)y * 1.75));
        G_COLOR = 0xf; G_BGCOLOR = 0;
        gfx_draw_icon(ICON(icon));
        delay_loop(2000.0);
        gfx_moveto(x, (int)((double)y * 1.75));
        G_COLOR = 0; G_BGCOLOR = 0xf;
        draw_unit_icon(ICON(icon));
        delay_loop(2000.0);
    }
}
void flash_war_icon(int x, int y) { flash_icon(x, y, 3); }                         // 1000:4384
void flash_nuke_icon(int x, int y, int c) { flash_icon(x, y, W(0x1a2e + NUKE_STAGE(c) * 2)); }   // 1000:44D9

// 1000:4126 - nuclear facility marker, with an announcement the first month it is seen.
void map_nuke_site(int c, int prev_stage, int seen_turn, int x1, int y1, int x2, int y2, int ix, int iy) {
    if (NUKE_STAGE(c) <= 6) return;
    gfx_moveto(ix, (int)((double)iy * 1.75));
    draw_unit_icon(ICON(W(0x1a2e + NUKE_STAGE(c) * 2)));
    if (seen_turn == TURN) return;
    if (!(prev_stage == 6 || (NUKE_STAGE(c) == 10 && prev_stage == 9))) return;
    text_box(x1, y1, x2, y2, S(0x1aa8), S(prev_stage == 6 ? 0x1a44 : 0x1a76), S(0x1aa6));
    flash_nuke_icon(ix, iy, c);
    delay_or_key(15000.0);
    gfx_copy_from_page(x1, x2, (int)((double)y1 * 1.75), (int)((double)y2 * 1.75));
}

void map_fallen_marker(int c, int x, int y) {     // 1000:42A5
    if (STATUS(c) != 10) return;
    gfx_moveto(x, (int)((double)y * 1.75));
    draw_unit_icon(ICON(4));
}

void map_war_marker(int a, int b, int seen_turn, int x, int y) {      // 1000:42F5
    if (WAR(a, b) == 0 || STATUS(a) >= 10 || STATUS(b) >= 10) return;
    gfx_moveto(x, (int)((double)y * 1.75));
    if (TURN != seen_turn) flash_war_icon(x, y);
    draw_unit_icon(ICON(3));
}

void map_overlay_libya(void) {                    // 1000:3D5E
    if (TURN == 1) { W(0x9198) = 1; W(0x918e) = W(0xa102); W(0x9190) = W(0xa100); }
    map_fallen_marker(2, 0xe8, 0x51);
    map_fallen_marker(1, 0x15e, 0xf);
    map_nuke_site(1, W(0x9190), W(0x9198), 0x32, 5, 0xfa, 0x3c, 0x111, 6);
    map_nuke_site(2, W(0x918e), W(0x9198), 0xf, 0x34, 0xbe, 0x58, 0x9c, 0x1f);
    map_war_marker(6, 2, W(0x9198), 0x53, 0x32);
    map_war_marker(2, 1, W(0x9198), 0x118, 0x2f);
    W(0x9198) = TURN; W(0x918e) = W(0xa102); W(0x9190) = W(0xa100);
}

void map_overlay_egypt(void) {                    // 1000:3E40
    if (TURN == 1) { W(0x919a) = 1; W(0x9192) = W(0xa108); W(0x9194) = W(0xa0fe); }
    map_fallen_marker(0, 0x189, 0x3a);
    map_fallen_marker(5, 0x3a, 0x30);
    map_nuke_site(5, W(0x9192), W(0x919a), 0x1e, 0x91, 0xe6, 0xbd, 0xd2, 0x79);
    map_war_marker(5, 0, W(0x919a), 0x11d, 0x52);
    map_nuke_site(0, W(0x9194), W(0x919a), 0xa5, 0x91, 0x16d, 0xbb, 0x17e, 0x89);
    W(0x919a) = TURN; W(0x9192) = W(0xa108); W(0x9194) = W(0xa0fe);
}

void map_overlay_syria(void) {                    // 1000:3F03
    if (TURN == 1) { W(0x919c) = 1; W(0x9196) = W(0xa10a); }
    map_nuke_site(6, W(0x9196), W(0x919c), 0x50, 0x61, 0x118, 0x91, 0xcd, 0x43);
    map_war_marker(6, 2, W(0x919c), 0x151, 0x4d);
    map_war_marker(7, 6, W(0x919c), 0x5a, 0x70);
    map_war_marker(6, 3, W(0x919c), 0x98, 0x76);
    map_fallen_marker(6, 0xcd, 0x43);
    map_fallen_marker(4, 0x4a, 0x4b);
    W(0x919c) = TURN; W(0x9196) = W(0xa10a);
}

void map_overlay_israel(void) {                   // 1000:3FC9
    map_fallen_marker(0, 0x3f, 0x8a);
    map_fallen_marker(6, 0x14f, 0x19);
    map_fallen_marker(3, 0x116, 0x6b);
    map_fallen_marker(4, 0xd0, 0x10);
    if (W(0x98b0) != 0) { gfx_moveto(0x5b, 0xee); draw_unit_icon(ICON(7)); }
    if (W(0x98bc) != 0) { gfx_moveto(0xfb, 0x32); draw_unit_icon(ICON(7)); }
    if (W(0x98b8) != 0) { gfx_moveto(0xd0, 0x1c); draw_unit_icon(ICON(7)); }
    if (W(0x98b6) != 0) { gfx_moveto(0xae, 0x93); draw_unit_icon(ICON(7)); }
    map_war_marker(7, 0, W(0x919e), 0x5b, 0x88);
    map_war_marker(7, 6, W(0x919e), 0xfb, 0x1d);
    map_war_marker(7, 4, W(0x919e), 0xd0, 0x10);
    map_war_marker(7, 3, W(0x919e), 0xae, 0x54);
    W(0x919e) = TURN;
}

u16 month_name(int m) {                           // 1000:4652 - near pointer to a month name
    return (m < 1 || m > 12) ? UW(0x1b0e) : UW(0x1b0e + m * 2);
}
