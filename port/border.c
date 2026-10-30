// Border command: the orders available on one of Israel's four borders, in peace and war
// (original 1000:8462 - 1000:9375).
#include <stdlib.h>
#include "conflict.h"

#define TURN          W(0xb268)
#define BRIGADES(c)   W(0x98b0 + (c) * 2)
#define POOL          W(0x98be)                   // brigades not assigned to a border
#define ORDER(c)      W(0xb2e8 + (c) * 2)         // order already given on this border this month
#define ACTION(c)     W(0xa12e + (c) * 2)         // strike / special action chosen against c

static void order_already_given(int c) {          // 1000:8CE6
    char *body = S(0xa2ba);
    strcpy(S(0xa3b4), S(0x3ad0));
    strcpy(S(0xa1f2), S(0x3ad2));
    strcpy(body, S(0x3ae9));
    strcat(body, S(ATWAR(c) == 0 ? weapon_name(ORDER(c) - 1) : weapon_blurb(ORDER(c) - 1)));
    text_box(0x140, 0x87, 0x276, 0xc3, S(0xa1f2), body, S(0xa3b4));
    menu(0x17c, 5, 0x276, 0x50, S(UW(0xcd0)), c);
    gfx_copy_from_page(0x17c, 0x276, 8, 0x8c);
    gfx_copy_from_page(0x140, 0x276, 0xec, 0x155);
}

static void border_closed(int c) {                // 1000:8DF1 - the neighbour no longer exists
    map_overlay_israel();
    menu(400, 5, 0x276, 0x3c, S(UW(0xcf0)), c);
    gfx_copy_from_page(400, 0x276, 8, 0x69);
}

static void post_brigade(int c) {                 // 1000:8E39
    BRIGADES(c)++; POOL--;
    if (F(7, 7, 1) > 0x95) { F(7, 7, 1) -= 0x96; F(7, c, 1) += 0x96; }
    if (F(7, 7, 4) > 199) { F(7, 7, 4) -= 200; F(7, c, 4) += 200; }
    if (F(7, 7, 5) > 9) { F(7, 7, 5) -= 10; F(7, c, 4) += 10; }
}

static void full_deployment(int c) {              // 1000:8EB4
    i16 n = (i16)((1000 - F(7, c, 0)) / 200);
    if (POOL < n) { BRIGADES(c) += POOL; POOL = 0; }
    else { BRIGADES(c) = 5; POOL -= n; }
    for (int k = 1; k < 9; k++) if (k != 8 && k != 2) {
        i16 m = (i16)((i16)(F(7, 7, k) << 1) / 3);
        F(7, 7, k) -= m; F(7, c, k) += m;
    }
}

static void withdraw(int c) {                     // 1000:8F4E
    POOL += BRIGADES(c); BRIGADES(c) = 0;
    for (int k = 1; k < 9; k++) { F(7, 7, k) += F(7, c, k); F(7, c, k) = 0; }
    REL(7, c)++;
}

static void pull_back_brigade(int c) {            // 1000:8FB1
    if (BRIGADES(c) != 0) { BRIGADES(c)--; POOL++; }
}

static void match_enemy(int c) {                  // 1000:8FCC - deploy just enough to match their army
    int enemy = F(c, c, 0) + F(c, 7, 0);
    if (BRIGADES(c) * 200 <= enemy) { BRIGADES(c) += POOL; POOL = 0; }
    else {
        int n = POOL;
        while (n * 200 > enemy) n--;
        BRIGADES(c) += (i16)n; POOL -= (i16)n;
    }
    for (int k = 1; k < 9; k++) if (k != 2 && k != 8) {
        i16 d = (i16)(F(7, c, k) - (F(c, c, k) + F(c, 7, k)));
        if (d < F(7, 7, k)) { F(7, c, k) += F(7, 7, k); F(7, 7, k) = 0; }
        else { F(7, 7, k) -= d; F(7, c, k) += d; }
    }
}

static void commit_everything(int c) {            // 1000:90E3
    BRIGADES(c) += POOL; POOL = 0;
    for (int k = 1; k < 9; k++) if (k != 8 && k != 2) { F(7, c, k) += F(7, 7, k); F(7, 7, k) = 0; }
}

static void reinforce_armour(int c) {             // 1000:9147
    i16 d = (i16)(F(c, 7, 1) - F(7, c, 1) + 0x32);
    if (d < 0) d = 0x32;
    if (F(7, 7, 1) < d) { F(7, c, 1) += F(7, 7, 1); F(7, 7, 1) = 0; }
    else { F(7, c, 1) += d; F(7, 7, 1) -= d; }
    if (F(7, 7, 4) < 0x32) { F(7, c, 4) += F(7, 7, 4); F(7, 7, 4) = 0; }
    else { F(7, c, 4) += 0x32; F(7, 7, 4) -= 0x32; }
}

static void reinforce_air_defence(int c) {        // 1000:921D
    i16 b = BRIGADES(c), t = (i16)(b * 10);
    if (t <= F(7, 7, 5)) { F(7, c, 5) += t; F(7, 7, 5) += (i16)(b * -10); }
    else { F(7, c, 5) += F(7, 7, 5); F(7, 7, 5) = 0; }
}

static void reinforce_air(int c) {                // 1000:9281
    i16 a = (i16)(F(c, 7, 3) - F(7, c, 3) + 0x14), s = (i16)(F(c, 7, 7) - F(7, c, 7) + 10);
    if (F(7, 7, 3) < a) { F(7, c, 3) += F(7, 7, 3); F(7, 7, 3) = 0; } else { F(7, c, 3) += a; F(7, 7, 3) -= a; }
    if (F(7, 7, 7) < s) { F(7, c, 7) += F(7, 7, 7); F(7, 7, 7) = 0; } else { F(7, c, 7) += s; F(7, 7, 7) -= s; }
    if (F(7, 7, 6) != 0) { i16 v = F(7, 7, 6); F(7, 7, 6) -= v / 2; F(7, c, 6) += v / 2; }
}

void border_command(int which) {                  // 1000:8462
    static const int border_country[4] = {0, 3, 4, 6};
    char spec[400];
    int opt[16], n = 0, c = 0;
    if (which >= 1 && which <= 4) c = border_country[which - 1];
    else puts(S(0x3a4c));
    if (W(0x9292) != TURN) { for (int i = 0; i < 8; i++) ORDER(i) = 0; W(0x9292) = TURN; }
    int rel = REL(7, c);
    if (STATUS(c) == 10) { border_closed(c); return; }
    if (ORDER(c) != 0) { order_already_given(c); if (ORDER(c) != 0) return; }
    if (ATWAR(c) == 0) {
        int b = BRIGADES(c);
        if (rel > 1 && b < 5 && POOL != 0) { n = 1; opt[0] = 1; }
        if ((rel < 4 || b != 0) && b < 5 && b + POOL > 4) opt[n++] = 2;
        if (rel < 4 && b > 0) opt[n++] = 3;
        if (rel < 5 && rel > 1 && F(7, 7, 2) != 0) {
            if (rel == 4) opt[n++] = 4;
            else if (NUKE_STAGE(c) < 7) opt[n++] = 5;
            opt[n++] = 6;
        }
        if (NUKE_STAGE(c) > 6 && F(7, 7, 2) != 0 && rel > 1) opt[n++] = 7;
        if (b > 0) opt[n++] = 8;
        if (rel == 1 && b == 0) opt[n++] = 9;
        if (b > 4) opt[n++] = 0xb;
        opt[n++] = 10;
    } else {
        if (POOL != 0) opt[0] = 1;
        n = (POOL != 0);
        if (F(7, 7, 1) > 0x32) opt[n++] = 2;
        if (F(7, 7, 5) > 0) opt[n++] = 3;
        if (F(7, 7, 3) > 0 || F(7, 7, 7) > 0 || F(7, 7, 6) > 0) opt[n++] = 4;
        if (n != 0 || F(7, 7, 4) != 0) opt[n++] = 5;
        if (ATWAR(c) > 1) opt[n++] = 6;
        if (NUKE_STAGE(7) == 10) opt[n++] = 7;
        opt[n++] = 8;
    }
    strcpy(spec, S(0x3a63));
    for (int i = 0; i < n; i++) {
        strcat(spec, S(ATWAR(c) == 0 ? weapon_name(opt[i] - 1) : weapon_blurb(opt[i] - 1)));
        strcat(spec, S(0x3a74));
    }
    char *title = S(0xa1f2), *body = S(0xa2ba), *extra = S(0xa3b4);
    strcpy(title, S(UW(0xd80 + c * 2))); strcat(title, S(0x3a76));
    strcpy(body, S(0x3a8e));
    if (ATWAR(c) != 0) strcat(body, S(UW(0xa1d2 + c * 2)));
    if (ATWAR(c) == 0) map_overlay_israel();
    strcat(body, S(front_report(c)));
    strcat(body, S(0x3a90));
    strcpy(extra, S(0x3a92));
    int choice;
    if (ATWAR(c) == 0) {
        text_box(300, 0x91, 0x276, 0xc3, title, body, extra);
        if ((int)strlen(spec) < 0x79) {
            if ((int)strlen(spec) < 0x51) choice = menu(0x1a4, 5, 0x276, 0x50, spec, c);
            else choice = menu(400, 5, 0x276, 100, spec, c);
        } else choice = menu(200, 5, 0x276, 0x8f, spec, c);
    } else {
        text_box(300, 0x8f, 0x276, 0xc3, title, body, extra);
        if (n == 8) choice = menu(0x154, 5, 0x276, 0x93, spec, c);
        else if ((int)strlen(spec) < 0x32) choice = menu(0x1a4, 5, 0x276, 0x50, spec, c);
        else if ((int)strlen(spec) < 0x4c) choice = menu(0x1a4, 5, 0x276, 0x8c, spec, c);
        else choice = menu(0x154, 5, 0x276, 0x8c, spec, c);
    }
    gfx_copy_from_page(200, 0x276, 8, 0x155);
    if (ATWAR(c) != 0) {
        gfx_copy_from_page(0x3c, 0x10e, 0x15, 0x11f);
        gfx_copy_from_page(5, 0x118, 0x126, 0x155);
    }
    int action = opt[choice - 1];
    ORDER(c) = (i16)action;
    if (ATWAR(c) == 0) {
        switch (action) {
        case 1: post_brigade(c); break;
        case 2: full_deployment(c); W(0xb264) += 5; break;
        case 3:                                   // open hostilities
            W(0x98a0 + c * 2) += REL(7, c) + 1;
            REL(7, c) = 1;
            if (rel != 1) relations_changed(7, c);
            W(0xb276) += REL(7, c) + 5;
            W(0xb264) += REL(7, c) * 5 + 10;
            break;
        case 4: case 5: case 6: case 7:           // air strikes of increasing severity
            ACTION(c) = (i16)(action - 3);
            W(0xb276) += 2;
            if (action - 3 == 4) W(0xb276) += 2;
            break;
        case 8: withdraw(c); break;
        case 9: match_enemy(c); break;
        case 10: ORDER(c) = 0; break;
        case 11: commit_everything(c); break;
        }
    }
    if (ATWAR(c) == 0) return;
    switch (action) {
    case 1: BRIGADES(c)++; POOL--; break;
    case 2: reinforce_armour(c); break;
    case 3: reinforce_air_defence(c); break;
    case 4: reinforce_air(c); break;
    case 5: commit_everything(c); break;
    case 6: pull_back_brigade(c); break;
    case 7:                                       // nuclear strike, after confirmation
        ACTION(c) = 5;
        if (menu(0xdc, 0x32, 0x1a4, 0x82, S(UW(0xcee)), 0) == 2) ACTION(c) = 0;
        else W(0xb276) += 10;
        gfx_copy_from_page(0xdc, 0x1a4, 0x57, 0xe3);
        break;
    case 8: ORDER(c) = 0; break;
    default: puts(S(0x3a94)); host_exit();
    }
}
