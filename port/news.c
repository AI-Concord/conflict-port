// News generation (headlines and country stories for every kind of event), the annual
// summit and budget review, and the starting position of each country
// (original segment 1D93).
#include "conflict.h"

#define TURN        W(0xb268)
#define WAR(a, b)   W(0x9310 + (a) * 0x10 + (b) * 2)
#define CONDEMNED   W(0xb264)
#define POOL        W(0x98be)
#define T(table, i) UW((table) + (i) * 2)         // entry of a template-pointer table

void relations_changed(int a, int b) {            // 1D93:0007
    u16 head = 0, story = 0;
    int wars = 0;
    int r6 = rnd(6), r = r6 - 1;
    int rel = (b < a) ? REL(a, b) : REL(b, a);
    if (rel == 4 && a == 7 && TURN == 2) head = (r < 4) ? UW(0x580a) : UW(0x580c);
    if (rel == 3) {
        story = UW(0x5c58);
        if (r < 2 || r > 5) head = UW(0x5810);
        else head = T(0x580a, r == 0 ? 1 : r);
        if (F(a, b, 9) == 1) { head = T(0x580a, r6 + 1); story = UW(0x5c56); }
        if (b == 1 || b == 5) head = UW(0x581a);
    }
    if (rel == 2) {
        head = T(0x5962, r6 + 1);
        if (a == 7 && F(7, b, 0) != 0) head = UW(0x5964);
        if (a == 7 && TURN < 5) head = UW(0x5962);
        if (a == 1 || a == 5) head = (r < 3) ? UW(0x5972) : UW(0x5974);
        story = UW(0x5c5a);
        if (a == 7 && TARGET(b) != 7) story = UW(0x5c5c);
    }
    if (rel == 1) {                               // war
        head = T(0x5ac6, r);
        if (a == 7 && TARGET(b) != 7) head = UW(0x5ac6);
        if (b == 4) head = UW(0x5ad2);
        if (a == 1 || a == 5) head = UW(0x5ad4);
        for (int i = 0; i < 8; i++) {
            if (b < i && WAR(i, b) != 0) wars++;
            if (i < b && WAR(b, i) != 0) wars++;
        }
        if (wars != 0) head = UW(0x5ad6);
        story = UW(0x5c5e);
    }
    if (rel == 8 || rel == 9) { r = rnd(3) - 1; story = T(0x5cec, r); }
    if (rel == 10) {                              // alliance
        if (F(a, b, 9) == 1 && r > 0 && r < 4) head = T(0x5b80, r);
        if (TARGET(a) == TARGET(b) && F(a, b, 9) != 0) { head = UW(0x5b88); story = UW(0x5d4c); }
        int ta = TARGET(a), tb = TARGET(b);
        if (ta == tb && F(a, ta, 9) == 1 && F(b, tb, 9) == 1) { head = UW(0x5b80); story = UW(0x5d4e); }
    }
    if (head) add_headline(head, a, b);
    if (story) add_story(story, a, b);
}

void unrest_news(int c) {                         // 1D93:037B
    u16 head = 0;
    if (c == 7) {
        int r = rnd(2);
        if (STATUS(7) > 1) {
            if (W(0xb26a) == 0) head = T(0x6048, STATUS(7) * 2 + r - 5);
            if (W(0xb26a) == 1) {
                head = T(0x6286, STATUS(7) - 1);
                if (WAR(7, 0) != 0 && WAR(7, 0) < 4) head = UW(0x6286);
            }
        }
    }
    int group = W(0xa13e + c * 2);
    if (group == 0xc || group == 0xd) head = T(0x5d9c, rnd(2) - 1);
    if (c != 7 && W(0xa13e + c * 2) < 7 && STATUS(c) > 4) head = T(0x610c, rnd(4) - 1);
    if (head) add_headline(head, c, c);
}

void relations_news(int a, int b) {               // 1D93:0468
    u16 head = 0;
    int r = rnd(6) - 1;
    int rel = (b < a) ? REL(a, b) : REL(b, a);
    if (rel == 1) head = T(0x64ac, r);
    if (rel == 4 && a != 7) {
        head = T(0x63c4, r);
        if (F(a, b, 9) == 1 && TARGET(a) == b) head = UW(0x63d0);
        if (F(a, b, 9) == 1 && UNREST(b) + 2 < UNREST(a)) head = UW(0x63d2);
    }
    if (rel == 7 && F(a, b, 9) == 1 && a != 7) head = T(0x6588, r);
    if (head) add_headline(head, a, b);
}

void battle_news(int a, int b, int kind, int amount) {     // 1D93:0599
    u16 head = 0, story = 0;
    int wars = 0;
    int months = (b < a) ? WAR(a, b) : WAR(b, a);
    if (kind == 0) {                              // infantry losses
        int r4 = rnd(4), r = r4 - 1;
        if (F(a, b, 0) < (i16)(amount * 2)) { head = T(0x662c, r); story = UW(0x6dea); }
        if (months == 1) {
            head = T(0x6704, r4 + 1); story = UW(0x6de2);
            if (a == 4) head = UW(0x6704);
            for (int i = 0; i < 8; i++) {
                if (WAR(i, a) != 0) wars++;
                if (WAR(a, i) != 0) wars++;
            }
            if (wars > 1) { head = UW(0x6706); story = UW(0x6de0); }
        }
        if (months > 1 && amount > 0x32) story = UW(0x6de4);
        if (months > 1 && amount > 400) { head = T(0x682a, r); story = UW(0x6de6); }
        if (months > 1 && amount > 0x2ee) { head = T(0x682a, r4 + 3); story = UW(0x6de8); }
        if (months == 6) head = T(0x68c6, r);
    }
    if (kind == 2) head = T(0x6e84, rnd(5) - 1);  // front line moved
    if (kind == 1) {                              // armour losses
        int r3 = rnd(3);
        if (months == 2) head = T(0x6ad8, r3 - 1);
        else {
            if (amount > 300) {
                head = T(0x6ad8, r3 + 2);
                if (F(a, b, 3) + 0x14 < F(b, a, 3)) head = T(0x6ad8, r3 + 0xb);
            }
            if (amount > 500) head = T(0x6ad8, r3 + 5);
            if (amount > 700) head = T(0x6ad8, r3 + 8);
        }
    }
    if ((a == 7 || b == 7) && kind == 0) {        // one-line summary for the border command report
        if (a == 7) {
            UW(0xa1d2 + b * 2) = UW(0x6c3e);
            if (amount > 0x32) UW(0xa1d2 + b * 2) = UW(0x6c40);
            if (amount > 0x96) UW(0xa1d2 + b * 2) = UW(0x6c42);
            if (amount > 0xfa) UW(0xa1d2 + b * 2) = UW(0x6c44);
            if (amount > 0x1c2) UW(0xa1d2 + b * 2) = UW(0x6c46);
            if (amount > 600) UW(0xa1d2 + b * 2) = UW(0x6c48);
        }
        if (b == 7) {
            UW(0xa1d2 + a * 2) = UW(0x6c4a);
            if (amount > 100) UW(0xa1d2 + a * 2) = UW(0x6c4c);
            if (amount > 300) UW(0xa1d2 + a * 2) = UW(0x6c4e);
            if (amount > 500) UW(0xa1d2 + a * 2) = UW(0x6c50);
        }
    }
    if (head) add_headline(head, a, b);
    if (story) add_story(story, a, b);
}

// 1D93:0931 - kind: 1 military, 2 industrial, 3 civilian, 4 nuclear target; result: 1 failed .. 4 devastating.
void strike_news(int kind, int result, int a, int b) {
    u16 head = 0, story = 0;
    int r;
    do r = rnd(6) - 1; while (r == W(0x92a6));    // never the same wording twice running
    W(0x92a6) = (i16)r;
    if (result == 1) head = T(0x6f6c, r);
    if (result == 2) head = T(0x706e, r);
    if (result == 3) {
        if (kind == 3) head = T(0x7150, r);
        if (kind == 1) head = UW(0x73f6);
        if (kind == 2) head = T(0x7336, r);
        if (kind == 3) story = UW(0x74b2);
        if (kind == 1 || kind == 2) story = UW(0x74b4);
    }
    if (result == 4) {
        if (kind == 3) head = T(0x7240, r);
        if (kind == 1) head = UW(0x741c);
        if (kind == 4) head = UW(0x74e0);
        if (kind == 2) {
            head = UW(0x73cc);
            if (b == 4) head = UW(0x73ce);
            if (b == 1) head = T(0x73cc, rnd(2) + 1);
        }
        if (kind == 3) story = UW(0x74b2);
        if (kind == 1 || kind == 2) story = UW(0x74b4);
        if (kind == 4) story = UW(0x74b6);
    }
    if (REL(a, b) == 1 || REL(b, a) == 1) head = T(0x75aa, r);
    if (head) add_headline(head, a, b);
    if (story) add_story(story, a, b);
}

void peace_news(int a, int b, int kind) {         // 1D93:0AD0
    u16 head = 0;
    if (kind == 1) head = UW(0x75d0);
    if (kind == 2) head = UW(0x75ea);
    if (head) add_headline(head, a, b);
}

void world_news(int kind) {                       // 1D93:0B2A
    u16 head = 0;
    int r = rnd(3) - 1;
    if (TURN == 1) W(0x92a8) = 0;
    if (kind == 1 && W(0x92a8) == 0) { head = UW(0x761c); W(0x92a8) = 1; }
    if (kind == 2 && W(0x92a8) != 2) { head = UW(0x7642); W(0x92a8) = 2; }
    if (kind == 3) {
        head = T(0x7732, r);
        if (CONDEMNED > 0x14) head = UW(0x7738);
        if (CONDEMNED > 0x32) head = UW(0x773a);
    }
    if (kind == 4) { head = T(0x781c, r); if (NUKE_STAGE(5) == 10) head = UW(0x7824); }
    if (kind == 5) head = T(0x78be, r);
    if (kind == 6) head = UW(0x78e6);
    if (head) add_headline(head, 7, 7);
}

static void pause_for_reading(void) {             // 1D93:1A01
    float f; memcpy(&f, DSP + 0xb27a, 4);
    long n = (long)(80000.0 / (double)f);
    for (long i = 0; i < n; i++) if (gfx_getkey() != 0) return;
}

void summit(void) {                               // 1D93:0C02 - the July peace summit
    char num[16], text[400];
    char *title = S(0xa1f2), *body = S(0xa2ba), *extra = S(0xa3b4);
    int before = CONDEMNED;
    strcpy(title, S(0x78e8)); strcpy(body, S(0x7900));
    text_box(0x96, 0x37, 0x1e0, 100, title, body, S(0x7963));
    if (menu(200, 0x6e, 0x1ae, 0xa0, S(UW(0xcf6)), 0) == 2) { CONDEMNED += 5; return; }    // "Not Attend"
    gfx_draw_map(0);
    recolor_fallen();
    for (int a = 0; a < 8; a++)                   // long wars are brought to the table
        for (int b = 0; b < a; b++) {
            if (!(WAR(a, b) > 2 && STATUS(a) < 10 && STATUS(b) < 10 && (STATUS(a) > 2 || a == 7) && STATUS(b) > 2)) continue;
            strcpy(title, S(0x7965));
            int_to_str(WAR(a, b), num);
            sprintf(extra, S(0x797d), num);
            if (a == 7) {
                strcpy(text, S(0x7999)); strcat(text, S(UW(0xd30 + b * 2))); strcat(text, S(0x79b2));
                text_box(0x82, 100, 500, 0xc3, title, text, extra);
                if (menu(0x1ae, 5, 0x276, 0x55, S(UW(0xcf2)), 0) == 1) make_peace(7, b);
                gfx_copy_from_page(0x82, 500, 0xaf, 0x155);
                gfx_copy_from_page(0x1ae, 0x276, 8, 0x94);
            } else {
                strcpy(body, S(0x7a51)); strcat(body, S(UW(0xd80 + a * 2))); strcat(body, S(0x7a56));
                strcat(body, S(UW(0xd80 + b * 2))); strcat(body, S(0x7a5a));
                text_box(0x96, 10, 0x1e0, 0x5a, title, body, extra);
                pause_for_reading();
                make_peace(a, b);
                gfx_copy_from_page(0x96, 0x1e0, 0x11, 0x9d);
            }
        }
    int host = 3;                                 // the strongest friendly neighbour proposes a Palestinian homeland
    for (int c = 0; c < 8; c++)
        if (F(7, c, 9) == 1 && REL(7, c) > 3 && STATUS(c) < 8 && POWER(host) <= POWER(c)) host = c;
    if (W(0xb26a) == 0 && REL(7, host) > 3 && STATUS(host) < 8) {
        strcpy(text, S(0x7a88)); strcat(text, S(UW(0xd30 + host * 2))); strcat(text, S(0x7af7));
        strcat(text, S(UW(0xd80 + host * 2))); strcat(text, S(0x7b86));
        strcpy(extra, S(0x7b8f));
        text_box(0x46, 100, 0x230, 0xc3, title, text, extra);
        if (menu(0x1a4, 5, 0x276, 0x50, S(UW(0xcde)), 1) == 1) {
            CONDEMNED -= 0x19;
            if (POWER(7) < 10) POWER(7)++;
            if (POWER(host) < 10) POWER(host)++;
            W(0x98a0 + host * 2) = -2;
            UNREST(7) = 0;
            POOL += W(0x98c0); W(0x98c0) = 0;
            W(0xb26a) = 1;
        }
    }
    gfx_draw_map(0);
    recolor_fallen();
    strcpy(title, S(0x7be3)); strcpy(body, S(0x7bf3)); strcpy(extra, S(0x7c87));
    text_box(0x6e, 0x78, 0x208, 0xc3, title, body, extra);
    int m = menu(0x1ae, 5, 0x276, 100, S(UW(0xce2)), 1);
    gfx_copy_from_page(0x6e, 0x208, 0xd2, 0x155);
    if (m == 1) { CONDEMNED -= 0xf; W(0xb26c) = 1; }
    if (before > 0x31 && CONDEMNED < 0x32) world_news(6);
}

void yearly_review(int year) {                    // 1D93:111F - January: US aid and the defence budget
    char ys[16], pct[16], num[16];
    char *title = S(0xa1f2), *body = S(0xa2ba), *extra = S(0xa3b4);
    int aid = 0;
    int_to_str(year, ys);
    int_to_str(BUDGET_PCT(7), pct);
    strcpy(title, ys); strcat(title, S(0x7cb5));
    strcpy(num, S(0x7ccd));
    if (CONDEMNED < 5) aid = 3000;
    if (CONDEMNED > 4 && CONDEMNED < 0x1e) aid = 2000;
    if (CONDEMNED > 0x1d && CONDEMNED < 0x32) aid = 1000;
    if (CONDEMNED > 0x31) aid = 0;
    MONEY(7) += (i16)aid;
    int_to_str(aid, num);
    strcpy(body, S(0x7cce));
    if (aid == 0) strcat(body, S(0x7d41));
    else { strcat(body, S(0x7d03)); strcat(body, num); strcat(body, S(0x7d37)); }
    int_to_str(MONEY(7), num);
    strcpy(extra, S(0x7d75)); strcat(extra, num); strcat(extra, S(0x7d90));
    text_box(0x14, 10, 0x172, 0x50, title, body, extra);
    strcpy(title, S(0x7d9b)); strcpy(body, S(0x7dad));
    strcpy(extra, S(0x7e07)); strcat(extra, pct); strcat(extra, S(0x7e31));
    text_box(0x172, 0x69, 0x276, 0xc3, title, body, extra);
    G_FONT = UW(0xb296);
    gfx_moveto(0x1ff, 0x130); gfx_text(S(0x7e33));
    gfx_moveto(0x1f2, 0xd8); gfx_text(S(0x7e35));
    G_FONT = UW(0xb294);
    int m = menu(400, 5, 0x276, 0x55, S(UW(0xce4)), 1);
    if (STATUS(2) != 10) gfx_copy_from_page(400, 0x276, 8, 0x96);
    gfx_copy_from_page(0x172, 0x276, 0xb7, 0x155);
    if (m == 1) { BUDGET_PCT(7) += 2; if (STATUS(7) > 5) STATUS(7)++; }
    if (m == 2) {
        BUDGET_PCT(7) -= 2;
        if (STATUS(7) > 0 && STATUS(7) < 6) STATUS(7)--;
        CONDEMNED -= 5;
    }
    if (W(0xb26c) == 0) {                         // offer to enlarge the army
        strcpy(title, S(0x7e37)); strcpy(body, S(0x7e41)); strcpy(extra, S(0x7e90));
        text_box(0x140, 0x78, 0x276, 0xc3, title, body, extra);
        m = menu(400, 5, 0x276, 0x5a, S(UW(0xce6)), 1);
        gfx_copy_from_page(0x140, 0x276, 0xd2, 0x155);
        if (m == 1) { POOL += 2; F(7, 7, 0) += 400; }
    }
    W(0xb26c) = 0;
}

#include "news_init.inc"

void init_countries(void) {                       // called in this order by the original
    init_egypt(); init_iran(); init_iraq(); init_jordan(); init_lebanon(); init_libya(); init_syria(); init_israel();
}
