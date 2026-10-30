// Policy screens (review forces, West Bank policing, nuclear programme), nuclear strikes,
// war endings, the final assessment and the main game loop
// (original 1000:A08F - 1000:B8AA).
#include <stdlib.h>
#include <setjmp.h>
#ifdef _WIN32            /* plain jump, no structured-exception unwinding (MinGW-w64) */
#undef setjmp
#define setjmp(buf) _setjmp((buf), NULL)
#endif
#include "conflict.h"

#define TURN       W(0xb268)
#define POOL       W(0x98be)
#define POLICING   W(0x98c0)                      // brigades policing the West Bank
#define TACTICS    W(0xb266)                      // 3 = soft, 6 = troops engage protesters
#define CONDEMNED  W(0xb264)                      // international condemnation
#define ATROCITIES W(0xb276)
#define WAR(a, b)  W(0x9310 + (a) * 0x10 + (b) * 2)
#define ON_ORDER(i, t)  W(0x98d2 + (i) * 100 + (t) * 2)
#define ICONS      UW(0xb29e)

static void force_line(int x, int y, int icon_kind, int value, u16 none_text, const char *suffix) {
    char *s = S(0xa3b4);
    int_to_str(value, s);
    if (value == 0 && none_text) strcpy(s, S(none_text));
    gfx_moveto(x, y);
    draw_unit_icon((const u8 *)S((u16)(W(0x3146 + icon_kind * 2) * 0x80 + ICONS)));
    gfx_moveto(x + 0x28, y + 7);
    if (suffix) strcat(s, suffix);
    gfx_text(s);
}

void forces_review(void) {                        // 1000:A08F
    char line[200], num[16], cost[16];
    char *title = S(0xa1f2), *body = S(0xa2ba), *list = S(0xa3b4);
    int n = 0, total;
    if (STATUS(1) == 10 || STATUS(2) == 10) recolor_fallen();
    strcpy(title, S(0x4786)); strcpy(body, S(0x479d)); strcpy(list, S(0x47ce)); strcpy(line, S(0x47cf));
    for (int i = 0; i < 0x13; i++) {              // deliveries due from last month's orders
        if (TURN > 1 && ON_ORDER(i, TURN - 1) != 0 && n < 7) {
            total = (i16)(ON_ORDER(i, TURN - 1) * W(0x411a + i * 2));
            strcat(line, S(UW(0x3cbc + i * 2)));
            while (gfx_text_width(line) < gfx_text_width(S(0x47d1))) strcat(line, S(0x47f5));
            int_to_str(ON_ORDER(i, TURN - 1), num);
            strcat(line, num);
            int_to_str(total, cost);
            while (gfx_text_width(line) < gfx_text_width(S(0x47f7))) strcat(line, S(0x4825));
            strcat(line, S(0x4827)); strcat(line, cost); strcat(line, S(0x4829));
            strcat(list, line);
            strcpy(line, S(0x482d));
            n++;
        }
    }
    if ((int)strlen(list) < 0x3d) {
        strcpy(title, S(0x482e));
        n = 0; total = 0;
        for (int i = 0; i < 0x13; i++) if (ON_ORDER(i, TURN) != 0) { total = (i16)(total + ON_ORDER(i, TURN) * W(0x411a + i * 2)); n++; }
        int_to_str(total, cost);
        if (total != 0) {
            if (n < 2) sprintf(body, S(0x4877), total); else sprintf(body, S(0x483a), n, total);
            text_box(100, 10, 400, 0x29, title, body, S(0x48ae));
        }
    } else text_box(0x28, 10, 0x1b8, 0x5a, title, body, list);
    strcpy(title, S(0x48b0));
    draw_titled_box(0x28, 100, 0x276, 0xc1, title);
    int_to_str(POOL, num);
    strcpy(list, num);
    total = (i16)(POOL * 0x14);
    strcat(list, S(total < 0x15 ? 0x48df : 0x48d2));
    int_to_str(total, num); strcat(list, num); strcat(list, S(0x48eb));
    if (total == 0) strcpy(list, S(0x4904));
    int_to_str(W(0x930e) / 10, num); strcat(list, num); strcat(list, S(0x4913));
    gfx_moveto(0x32, 199);
    draw_unit_icon((const u8 *)S((u16)(W(0x3146) * 0x80 + ICONS)));
    gfx_moveto(0x5a, 0xce);
    gfx_text(list);
    force_line(0x32, 0xfc, 1, F(7, 7, 1), 0x4927, NULL);
    force_line(0x32, 0x130, 3, F(7, 7, 3), 0x492c, NULL);
    force_line(0x96, 0xfc, 5, F(7, 7, 5), 0x4931, NULL);
    force_line(0x96, 0x130, 4, F(7, 7, 4), 0x4936, NULL);
    force_line(0xfa, 0xfc, 7, F(7, 7, 7), 0x493b, NULL);
    if (F(7, 7, 6) != 0) force_line(0xfa, 0x130, 6, F(7, 7, 6), 0, NULL);
    if (F(7, 7, 2) != 0) force_line(0x1a4, 0xea, 2, F(7, 7, 2), 0, S(0x4940));
    if (W(0x989c) != 0) {
        gfx_moveto(0x1a4, 0x10f);
        draw_unit_icon((const u8 *)S((u16)(W(0x3156) * 0x80 + ICONS)));
        gfx_moveto(0x1cc, 0x118);
        strcpy(list, S(0x494f)); gfx_text(list);
    }
    if (NUKE_STAGE(7) == 10) {
        gfx_moveto(0x1a4, 0x132);
        draw_unit_icon((const u8 *)S((u16)(ICONS + 0x100)));
        gfx_moveto(0x1cc, 0x139);
        strcpy(list, S(0x4965)); gfx_text(list);
    }
    menu(0x1c2, 5, 0x276, 0x28, S(UW(0xcc8)), 0);
}

void policing_menu(void) {                        // 1000:A86D - West Bank policing
    char *title = S(0xa1f2), *body = S(0xa2ba), *extra = S(0xa3b4);
    strcpy(title, S(0x4a76));
    strcpy(body, S(0x4a87)); strcat(body, S(UW(0x4a60 + STATUS(7) * 2))); strcat(body, S(0x4a95));
    strcpy(extra, S(0x4a97)); strcat(extra, S(POLICING == 0 ? 0x4aba : 0x4aab));
    strcat(extra, S(0x4ac7)); strcat(extra, S(TACTICS == 3 ? 0x4ad2 : 0x4ada));
    gfx_draw_map(1);
    text_box(0x140, 0x78, 0x276, 0xc3, title, body, extra);
    if (W(0x92a0) == TURN) { menu(0x1a4, 5, 0x276, 0x46, S(UW(0xcc2)), 0); return; }
    if (POLICING == 0 && POOL != 0) {
        if (menu(400, 5, 0x276, 100, S(UW(0xcac)), 0) == 1) { POOL--; POLICING++; }
        W(0x92a0) = TURN; return;
    }
    if (POLICING == 0 && POOL == 0) { menu(400, 5, 0x276, 0x46, S(UW(0xcd6)), 1); W(0x92a0) = TURN; return; }
    if (POLICING != 0 && TACTICS == 3) {
        int m = menu(400, 5, 0x276, 0x6e, S(UW(0xcd2)), 0);
        if (m == 2) { TACTICS = 6; CONDEMNED += 0xf; ATROCITIES += 3; world_news(5); W(0x92a0) = TURN; return; }
        if (m == 1) { POLICING--; POOL++; W(0x92a0) = TURN; return; }
    }
    if (POLICING != 0 && TACTICS == 6) {
        int m = menu(400, 5, 0x276, 100, S(UW(0xcd4)), 0);
        if (m == 2) TACTICS = 3;
        if (m == 1) { POLICING--; POOL++; }
        W(0x92a0) = TURN;
    }
}

void nuclear_policy(void) {                       // 1000:AAC8 - fund Israel's own programme
    char *title = S(0xa1f2), *body = S(0xa2ba), *extra = S(0xa3b4);
    int worst = 0, best = 0;
    if (STATUS(1) == 10 || STATUS(2) == 10) recolor_fallen();
    int stage = NUKE_STAGE(7);
    if (W(0x92a2) == TURN) stage = W(0x92a4);
    int luck = rnd(10);
    strcpy(title, S(0x4ae2));
    strcpy(extra, S(0x4afc));
    for (int c = 0; c < 7; c++) {                 // name the most dangerous neighbour
        int threat = NUKE_STAGE(c) * 5;
        if (F(7, c, 9) == 1 && STATUS(c) != 10) threat += 0x14;
        threat -= REL(7, c);
        if (best < threat) { worst = c; best = threat; }
    }
    strcat(extra, S(UW(0xd30 + worst * 2))); strcat(extra, S(0x4b15));
    if (stage == 6) { strcpy(body, S(0x4b18)); strcat(body, S(0x4b55)); }
    if (stage == 7) { strcpy(body, S(0x4b89)); strcat(body, S(0x4bca)); }
    if (stage == 8) { strcpy(body, S(0x4bfc)); strcat(body, S(0x4c3b)); }
    if (stage == 9) strcpy(body, S(0x4c5a));
    text_box(0x140, 0x73, 0x276, 0xc3, title, body, extra);
    if (W(0x92a2) == TURN) menu(0x1ae, 5, 0x276, 0x32, S(UW(0xccc)), 0);
    else if (NUKE_STAGE(7) == 9) { menu(0x1ae, 5, 0x276, 0x32, S(UW(0xce8)), 0); NUKE_STAGE(7) = 10; }
    else if (menu(0x1ae, 5, 0x276, 0x3c, S(UW(0xcca)), 0) == 1 && MONEY(7) > 0) {
        if (MONEY(7) < 0xf) luck++;
        if (MONEY(7) < 6) luck++;
        if (MONEY(7) < 0x15) MONEY(7) = 0; else MONEY(7) -= 0x14;
        if (NUKE_STAGE(7) < 10 && luck < 4) NUKE_STAGE(7)++;
        if (NUKE_STAGE(7) == 10) world_news(4);
    }
    W(0x92a2) = TURN;
    W(0x92a4) = (i16)stage;
}

static void final_assessment(void);
static void final_papers(int which);

// 1000:B5B5 - the game is over: last headline, verdict and return to the title menu.
void game_over(int reason, int paper, int wallpaper, int a, int b) {
    (void)b;
    add_headline(UW(0x5144 + (rnd(3) + reason * 3 - 1) * 2), a, a);
    index_news_by_country();
    strcpy(S(0xa1f2), S(0x5652));
    final_papers(paper);
    icon_wallpaper(wallpaper);
    strcpy(S(0xa2ba), S(UW(0x5362 + reason * 2)));
    strcpy(S(0xa3b4), S(0x5669));
    text_box(0x5a, 0x82, 0x23a, 0xc3, S(0xa1f2), S(0xa2ba), S(0xa3b4));
    final_assessment();
    game_longjmp(10);
}

void nuclear_strike(int a, int b) {               // 1000:AD9B - a uses nuclear weapons on b
    int r = rnd(5);
    F(b, a, 0) = 0;
    if (a == 7) CONDEMNED += 100;
    add_headline(UW(0x4d76 + (r - 1) * 2), a, b);
    add_headline(UW(0x4d76 + r * 2), a, b);
    add_story(UW(0x4d76), a, b);
    if ((NUKE_STAGE(a) == 10 && NUKE_STAGE(b) == 10) || W(0xb262) < 0x87) {     // escalation ends everything
        if (a == 7 || b == 7) game_over(3, 2, 2, 0, 0);
        else game_over(4, 2, 2, 0, 0);
    }
}

void war_ends(int winner, int loser) {            // 1000:AE6C - headlines when a country is defeated
    int wars = 0;
    for (int i = 0; i < 8; i++) {
        if (i < loser && WAR(loser, i) != 0) wars++;
        if (loser < i && WAR(i, loser) != 0) wars++;
    }
    for (int i = 0; i < 8; i++)
        if (i != winner && STATUS(i) < 10 && ((loser < i && REL(i, loser) < 3) || (i < loser && REL(loser, i) < 3)))
            add_story(UW(0x4e74), i, loser);
    u16 head = UW(0x4e6a);
    if (wars > 1) head = UW(0x4e68);
    if (loser == 4) head = UW(0x4e72);
    if (W(0xa12e + loser * 2) == 5) head = UW(0x4e70);
    add_headline(head, winner, loser);
    add_story(UW(0x4e6c), winner, loser);
    add_story(UW(0x4e6e), winner, loser);
}

static void final_papers(int which) {             // 1000:AF8A
    if (W(0xb25c) != 0) {
        draw_newspapers();
        strcpy(S(0xa2ba), S(UW(0x4f00 + which * 2)));
        menu(0x1ce, 5, 0x276, 0x2a, S(0xa2ba), 0);
    }
    G_COLOR = 0xe;
    gfx_fillrect(0, 0x27f, 0, 0x15d);
    G_COLOR = 0;
}

static void final_assessment(void) {              // 1000:AFF1 - score and summary of the player's rule
    char digits[16];
    char *body = S(0xa2ba), *s = S(0xa3b4);
    int bonus = 0, conquests = 0;
    if (STATUS(3) == 10 && STATUS(6) == 10 && STATUS(0) == 10 && STATUS(4) == 10) bonus = 100;
    if (bonus < 100 && (STATUS(6) == 10 || STATUS(0) == 10)) bonus += 0x32;
    int score = bonus + STATUS(7) * -8 - CONDEMNED + POWER(7) * 0x10 + TURN * 4;
    if (W(0x92f8) != 7) score -= 5;
    if (W(0x92f6) != 7) score -= 10;
    if (W(0x92fc) != 7) score -= 0x19;
    if (TURN < 4) score -= 0x14;
    score = (i16)score;
    int_to_str(score, digits);
    for (int i = 0; i < (int)strlen(digits); i++) digits[i] = (char)(digits[i] + '0');   // large numerals in the symbol font
    strcpy(body, S(0x53ba));
    strcpy(s, S(0x53cf));
    if (score < 0x1e) strcat(body, S(0x53d0)); else if (score < 0x2d) strcat(body, S(0x53e1));
    if (score > 0x2c && score < 0x41) strcat(body, S(0x53f2));
    if (score > 0x40 && score < 0x50) strcat(body, S(0x53ff));
    if (score > 0x4f && score < 0x96) strcat(body, S(0x540e));
    if (score > 0x95 && score < 0xa5) strcat(body, S(0x541e));
    if (score > 0xa4 && score < 0x10e) strcat(body, S(0x5430));
    if (score > 0x10d && score < 0x14a) strcat(body, S(0x5441));
    if (score > 0x149 && score < 400) strcat(body, S(0x544c));
    if (score > 399) strcat(body, S(0x5455));
    if (STATUS(6) == 10 && W(0x92fc) == 7) conquests = 1;
    if (STATUS(4) == 10 && W(0x92f8) == 7) conquests++;
    if (STATUS(3) == 10 && W(0x92f6) == 7) conquests++;
    if (STATUS(0) == 10 && W(0x92f0) == 7) conquests++;
    if (TURN < 6) sprintf(s, S(0x5461), (int)TURN);
    if (TURN > 5 && TURN < 0xe) sprintf(s, S(0x5480), (int)TURN);
    if (TURN > 0xd && TURN < 0x18) sprintf(s, S(0x54a4), TURN % 0xc);
    if (TURN > 0x17) strcpy(s, S(0x54cb));
    if (conquests == 0) strcat(s, S(0x5551));
    else {
        if (conquests == 1) strcat(s, S(0x54f3));
        if (conquests == 2) strcat(s, S(0x550d));
        if (conquests == 3) strcat(s, S(0x5524));
        if (conquests == 4) strcat(s, S(0x553d));
    }
    strcat(s, S(0x5567)); strcat(s, S(0x557f));
    if (ATROCITIES < 1) strcat(s, S(0x5590));
    if (ATROCITIES > 0 && ATROCITIES < 4) strcat(s, S(0x5593));
    if (ATROCITIES > 3 && ATROCITIES < 10) strcat(s, S(0x5597));
    if (ATROCITIES > 10) strcat(s, S(0x55a0));
    strcat(s, S(0x55a8)); strcat(s, S(0x55bb));
    if (CONDEMNED < 1) strcat(s, S(0x55da));
    if (CONDEMNED > 0 && CONDEMNED < 5) strcat(s, S(0x55e5));
    if (CONDEMNED > 4 && CONDEMNED < 0x1e) strcat(s, S(0x55eb));
    if (CONDEMNED > 0x1d) strcat(s, S(0x55f1));
    strcat(s, S(0x55f7)); strcat(s, S(UW(0x1098 + POWER(7) * 2))); strcat(s, S(0x5610)); strcat(s, S(0x5612));
    int verdict = 7;
    if (ATROCITIES > 10) verdict = 1;
    if (ATROCITIES > 0x14) verdict = 0;
    if (W(0xb26a) == 0 && W(0xb26c) == 0 && TACTICS == 6 && ATROCITIES > 8 && STATUS(7) > 4 && BUDGET_PCT(7) > 0x22 && CONDEMNED > 0x28) verdict = 5;
    if (ATROCITIES == 0 && CONDEMNED < 10) verdict = 4;
    if (ATROCITIES < 6 && CONDEMNED < 0x1e && STATUS(7) < 4 && (W(0xb26c) != 0 || W(0xb26a) != 0) && BUDGET_PCT(7) < 0x24) verdict = 2;
    if (ATROCITIES > 0 && ATROCITIES < 8 && W(0xb26a) == 0 && W(0xb26c) == 0 && POWER(7) > 4) verdict = 6;
    if (ATROCITIES < 4 && W(0xb26a) != 0 && W(0xb26c) != 0 && CONDEMNED < 0x1e) verdict = 3;
    strcat(s, S(UW(0x53aa + verdict * 2)));
    strcat(s, S(0x5627));
    strcat(body, S(0x562a));
    text_box(0x34, 10, 0x24e, 0x78, S(0x563e), body, s);
    G_FONT = UW(0xb296);
    gfx_moveto(0x214, 0x21);
    gfx_text(digits);
    G_FONT = UW(0xb294);
}

jmp_buf game_restart;                             // target of game_longjmp() in the stand-alone build

void game_main(void) {                            // 1000:B681
    G_COLOR = 0; G_BGCOLOR = 0xf;
    game_srand((u16)host_clock_cs());
    intro_screen();
    gfx_load_maps();
    gfx_load_pictures();
    calibrate_delay();
    {
        float f; memcpy(&f, DSP + 0xb27a, 4);
        if ((double)f < 1.0) delay_or_key(50000.0);       // let the opening text be read
    }
    int why = setjmp(game_restart);
    if (why != 0) {                               // back from a finished or abandoned game
        W(0xb270) = 0;
        G_FONT = UW(0xb294);
        int m = (why == 10) ? menu(0x1e0, 0x28, 0x276, 0x5a, S(0x5690), 0) : menu(200, 0x46, 400, 0x78, S(UW(0xcec)), 0);
        if (m == 2) { text_mode(); host_exit(); }
    }
    G_COLOR = 0xe;
    gfx_fillrect(0, 0x27f, 0, 0x15d);
    G_COLOR = 0;
    icon_wallpaper(99);
    title_screen();
    for (;;) {
        if (W(0xb278) != 0) { load_game(); if (W(0xb278) != 0) break; }
        new_game_init();
        ai_month_begins(1);
        for (int c = 0; c < 7; c++) country_turn(c);
        world_events();
        if (W(0xb278) == 0) break;
    }
    for (;;) {                                    // one pass per month
        index_news_by_country();
        receive_deliveries();
        monthly_briefing();
        thinking_start();
        redeploy_border_troops();
        for (int i = 0; i < 0x9c4; i++) B(0xa4ae + i) = 0;
        W(0xb25a) = 0; W(0xb260) = 0;
        covert_operations();
        ai_month_begins(0);
        for (int c = 0; c < 7; c++) if (STATUS(c) != 10) country_turn(c);
        war_month();
        hostile_acts();
        world_events();
        TURN++;
    }
}
