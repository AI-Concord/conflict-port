// Border fronts: the status report for one front and the strategic-action menu
// (original 1000:76F4 - 1000:8461).
#include "conflict.h"

#define TURN          W(0xb268)
#define FRONT_LINE(c) W(0x98a0 + (c) * 2)         // -5..+5: how the battle on this front is going
#define FRONT_SHOWN(c) W(0x9282 + (c) * 2)
#define BRIGADES(c)   W(0x98b0 + (c) * 2)         // Israeli brigades posted on the border with c
#define FONT_A UW(0xb294)
#define FONT_B UW(0xb296)

u16 front_report(int c) {                         // 1000:76F4 - returns a near pointer to the report text
    char *out = S(0x91b4), *num = S(0x927c), *bar = S(0x33ae);
    int row = 0x19;
    if (TURN == 1) for (int i = 0; i < 200; i++) B(0x91b4 + i) = 0;
    if (ATWAR(c) != 0) {
        if (ATWAR(c) == 1) { FRONT_SHOWN(c) = 0; strcpy(bar, S(0x33b9)); }
        int men = F(c, 7, 0) - F(7, c, 0), tanks = F(c, 7, 1) - F(7, c, 1), air = F(c, 7, 3) - F(7, c, 3), sams = F(c, 7, 5);
        strcpy(out, S(0x33c4));
        if (air > 10 && air < 0x14 && ATWAR(c) < 3) strcpy(out, S(0x33c5));
        if (air > 10 && air < 0x14 && ATWAR(c) > 2) strcpy(out, S(0x33e9));
        if (air > 0x13 && air < 0x1e) strcpy(out, S(0x3413));
        if (air > 0x1d) strcpy(out, S(0x3431));
        if (air > 10 && F(7, c, 3) < air) strcpy(out, S(0x345a));
        if (men > 100 && tanks < 200) strcat(out, S(0x347c));
        if (tanks > 200 && men < 100) strcat(out, S(0x349e));
        if (sams > 0x32 && air < 0) strcat(out, S(0x34b2));
        if (men > 100 && tanks > 200) strcat(out, S(0x34e3));
        draw_titled_box(0x3c, 0xc, 0x8e, 0xa4, S(0x3508));          // Israeli column
        for (int k = 0; k < 8; k++) {
            if (k == 2 || (k == 6 && F(7, c, 6) == 0)) continue;
            double dy = (double)row * 1.75;
            gfx_moveto(0x40, (int)dy);
            draw_unit_icon((const u8 *)S((u16)(W(0x3146 + k * 2) * 0x80 + UW(0xb29e))));
            gfx_moveto(0x68, (int)(dy + 6.0));
            int_to_str(F(7, c, k), num);
            if (F(7, c, k) == 0) strcpy(num, S(0x350f));
            gfx_text(num);
            row += 0x14;
        }
        row = 0x19;
        draw_titled_box(0xbe, 0xc, 0x110, 0xa4, S(UW(0xd30 + c * 2)));   // enemy column (rounded estimates)
        for (int k = 0; k < 8; k++) {
            if (k == 2 || (k == 6 && F(c, 7, 6) == 0)) continue;
            double dy = (double)row * 1.75;
            gfx_moveto(0xc1, (int)dy);
            draw_unit_icon((const u8 *)S((u16)(W(0x3146 + k * 2) * 0x80 + UW(0xb29e))));
            gfx_moveto(0xea, (int)(dy + 6.0));
            int v = (F(c, 7, k) / 10) * 10;
            int_to_str(v, num);
            if (v == 0) strcpy(num, S(0x3514));
            gfx_text(num);
            row += 0x14;
        }
        draw_titled_box(0x32, 0xa8, 0x118, 0xc3, S(0x3519));       // tug-of-war bar
        gfx_moveto(0x36, 0x142); gfx_text(S(0x3524));
        gfx_moveto(0xd4, 0x142); gfx_text(S(UW(0xd30 + c * 2)));
        gfx_moveto(0x6c, 0x142);
        G_FONT = FONT_B;
        gfx_text(bar);
        if (FRONT_SHOWN(c) != FRONT_LINE(c)) {                      // flash the old marker before moving it
            u8 save = G_COLOR;
            for (int n = 0; n < 8; n++) {
                G_COLOR = 0xf;
                for (int j = 0; j < 7; j++)
                    gfx_hline((FRONT_SHOWN(c) + 4) * 10 + 0x6c, (FRONT_SHOWN(c) + 6) * 10 + 0x6c, (int)(322.0 + (double)j), 1);
                host_delay_ms(35);
                gfx_moveto(0x6c, 0x142);
                G_COLOR = 0;
                gfx_text(bar);
                host_delay_ms(35);
            }
            G_COLOR = save;
        }
        {
            u8 save = G_COLOR;
            G_COLOR = 0xf;
            for (int j = 0; j < 7; j++) gfx_hline(0x6c, 0xd2, (int)(322.0 + (double)j), 1);
            G_COLOR = save;
        }
        strcpy(bar, S(0x352b));
        B(0x33b2 + FRONT_LINE(c)) = 0x3e;
        B(0x33b3 + FRONT_LINE(c)) = 0x3c;
        gfx_moveto(0x6c, 0x142);
        gfx_text(bar);
        G_FONT = FONT_A;
        FRONT_SHOWN(c) = FRONT_LINE(c);
    }
    if (ATWAR(c) == 0) {
        strcpy(out, S(0x3536)); strcat(out, S(UW(0xd80 + c * 2))); strcat(out, S(0x353b));
        int v = F(c, 7, 0);
        if (v == 0) strcat(out, S(0x3542));
        if (v > 0 && v < 500) strcat(out, S(0x3550));
        if (v > 499 && v < 1000) strcat(out, S(0x356a));
        if (v > 999) strcat(out, S(0x358b));
        v = F(7, c, 0);
        if (v == 0) strcat(out, S(0x35a7));
        if (v > 200 && v < 1000) {
            int_to_str(BRIGADES(c), num);
            strcat(out, S(0x35cb)); strcat(out, num); strcat(out, S(0x35d4));
        }
        if (v > 4 && v < 0xc9) strcat(out, S(0x35f4));
        if (v > 999) strcat(out, S(0x361d));
    }
    return 0x91b4;
}

void strategic_menu(void) {                       // 1000:7E9C - choose a border to act on
    int choice = 0, fronts = 0, target = 0;       // 'target' was uninitialised in the original
    int reserve = W(0x930e);
    char *title = S(0xa1f2), *body = S(0xa2ba), *extra = S(0xa3b4);
    if (ATWAR(0) != 0 && STATUS(0) != 10) fronts = 1;
    if (ATWAR(3) != 0 && STATUS(3) != 10) fronts++;
    if (ATWAR(4) != 0 && STATUS(4) != 10) fronts++;
    if (ATWAR(6) != 0 && STATUS(6) != 10) fronts++;
    strcpy(extra, S(0x3641));
    gfx_draw_map(1);
    map_overlay_israel();
    while (choice < 5) {
        strcpy(title, S(0x3642));
        if (fronts == 0 && W(0xb26e) == 7) {
            int r = rnd(6);
            if (r > 3) strcpy(body, S(0x3660));
            if (r == 1) strcpy(body, S(0x3685));
            if (r == 2) strcpy(body, S(0x36ac));
            if (r == 3) strcpy(body, S(0x36cc));
        }
        if (fronts == 1) strcpy(body, S(0x36d7));
        if (fronts == 2) { strcpy(body, S(0x36f0)); strcat(body, S(0x371f)); }
        if (fronts > 2) strcpy(body, S(0x3747));
        int ev = W(0xb26e);
        if (ev != 7) {
            if (ev < 100) { strcpy(body, S(0x3785)); strcat(body, S(UW(0xd80 + ev * 2))); strcat(body, S(0x37ad)); }
            if (ev > 99 && ev < 200) { strcpy(body, S(0x37be)); strcat(body, S(UW(0xd80 + (ev - 100) * 2))); strcat(body, S(0x37e5)); }
            if (ev > 199 && ev < 300) { strcpy(body, S(0x37f6)); strcat(body, S(UW(0xd80 + (ev - 200) * 2))); strcat(body, S(0x3821)); }
            if (ev > 299) { strcpy(body, S(0x3833)); strcat(body, S(UW(0xd80 + (ev - 300) * 2))); strcat(body, S(0x3868)); }
            if (STATUS(7) > 2) strcat(body, S(0x3873));
        }
        if (ev == 7) {
            for (int c = 0; c < 7; c++) {
                if (REL(7, c) == 2 && STATUS(c) < 10 && W(0x98b0 + c * 2) == 0) { strcpy(extra, S(0x388f)); strcat(extra, S(UW(0xd80 + c * 2))); strcat(extra, S(0x38bd)); }
                if (REL(7, c) == 1 && F(7, c, 0) != 0 && ATWAR(c) == 0) { strcpy(extra, S(0x38c8)); strcat(extra, S(UW(0xd30 + c * 2))); strcat(extra, S(0x38e6)); }
                if (W(0x989c) != 0 && TARGET(c) == 7 && REL(7, c) == 3 && STATUS(c) != 10) {
                    strcpy(extra, S(0x3900)); strcat(extra, S(0x391d)); strcat(extra, S(UW(0xd80 + c * 2))); strcat(extra, S(0x392f));
                }
                if (F(c, 7, 0) != 0 && W(0x98b0 + c * 2) == 0) {
                    strcpy(extra, S(0x3938)); strcat(extra, S(0x3959)); strcat(extra, S(UW(0xd80 + c * 2))); strcat(extra, S(0x3969));
                }
            }
        }
        if (reserve < 1000) { strcpy(body, S(0x3972)); strcat(body, S(0x39aa)); }
        gfx_copy_from_page(10, 0x276, 8, 0x155);
        map_overlay_israel();
        if ((int)strlen(body) < 0x47) text_box(0x140, 0x84, 0x276, 0xc3, title, body, extra);
        else text_box(0xe6, 0x6e, 0x276, 0xc3, title, body, extra);
        W(0xb270) = 4;
        choice = menu(0x1ae, 5, 0x276, 100, S(UW(0xcce)), 0);
        W(0xb270) = 0;
        gfx_copy_from_page(0xe6, 0x276, 0xc0, 0x155);
        gfx_copy_from_page(0x1ae, 0x276, 8, 0xaf);
        map_overlay_israel();
        if (choice < 5) {
            border_command(choice);
            if (choice == 1) target = 0;
            if (choice == 2) target = 3;
            if (choice == 3) target = 4;
            if (choice == 4) target = 6;
            strcpy(extra, S(0x39db));
            if (W(0xa12e + target * 2) != 0) strcpy(extra, S(0x39dc));
            if (W(0xa12e + target * 2) == 5) strcpy(extra, S(0x3a16));
        }
    }
}
