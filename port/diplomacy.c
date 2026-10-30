// Diplomatic and intelligence screens for one country, and the covert operations
// they set in motion (original 1000:61B0 - 1000:76F3).
#include "conflict.h"

#define TURN       W(0xb268)
#define WAR(a, b)  W(0x9310 + (a) * 0x10 + (b) * 2)
#define PENDING_REL(c)    W(0xa0de + (c) * 2)
#define PENDING_UNREST(c) W(0xa0ee + (c) * 2)
#define PEACE_TALKS(c)    W(0xa15e + (c) * 2)
#define COVERT_ORDER(c)   W(0xb2d6 + (c) * 2)     // 1 = assassinate leader, 2 = start coup
#define CONQUEROR(c)      W(0x92f0 + (c) * 2)     // for a fallen country: who took it (8 = internal collapse)

static void notice(u16 title, u16 body, u16 extra) {     // pop-up shown until a key or a short time-out
    text_box(0x50, 0x1e, 0x15e, 0x5f, S(title), S(body), S(extra));
    delay_or_key(22000.0);
    gfx_copy_from_page(0x50, 0x15e, 0x34, 0xa6);
}

void diplomacy_screen(int p) {                    // 1000:61B0
    char buf[400];
    int wars = 0;
    memset(buf, 0, 100);
    int rel = REL(7, p);
    char *title = S(0xa1f2), *extra = S(0xa3b4);
    strcpy(title, S(0x2820)); strcat(title, S(UW(0xd80 + p * 2))); strcat(title, S(0x2821));
    if (rel < 2) {
        strcpy(extra, S(0x2860)); strcat(extra, S(UW(0xd30 + p * 2))); strcat(extra, S(0x2883));
    } else {
        strcpy(extra, S(0x2837)); strcat(extra, S(UW(0xd30 + p * 2))); strcat(extra, S(0x284c));
        strcat(extra, S(UW(0xe08 + rel * 2))); strcat(extra, S(0x285d));
    }
    int tgt = TARGET(p);
    u16 t = UW(0x2806);
    if (tgt == 7 && rel > 4) t = UW(0x2808);
    if (rel == 9 && POWER(p) <= POWER(7)) t = UW(0x280a);
    if (rel == 9 && POWER(7) < POWER(p)) t = UW(0x280c);
    if (rel > 6)
        for (int i = 0; i < 8; i++)
            if ((i < p && WAR(p, i) != 0) || (p < i && WAR(i, p) != 0)) wars++;
    if (wars != 0 && rel < 10) t = UW(0x280e);
    if (rel > 3 && F(7, p, 0) > 0) t = UW(0x2812);
    if (PEACE_TALKS(p) == 0 && ATWAR(p) == 1) t = UW(0x2814);
    if (PEACE_TALKS(p) == 1) t = UW(0x2816);
    if (PEACE_TALKS(p) == 2) t = UW(0x2818);
    if (STATUS(p) == 10) t = UW(0x281a);
    if (PENDING_REL(p) != 0 && PENDING_REL(p) < rel) t = UW(0x281e);
    if (PENDING_REL(p) != 0 && rel < PENDING_REL(p)) t = UW(0x281c);
    expand_template(t, p, p, buf);
    if (REL(7, tgt) > 6 && rel > 6) {
        strcpy(buf, S(0x289a)); strcat(buf, S(UW(0xd80 + p * 2))); strcat(buf, S(0x289f));
        strcat(buf, S(UW(0xd30 + tgt * 2))); strcat(buf, S(0x28d0));
    }
    text_box(0x140, 0x78, 0x276, 0xbe, title, buf, extra);
    if (STATUS(p) == 10) {
        menu(0x1c2, 5, 0x276, 0x3c, S(UW(0xcdc)), p);
        gfx_copy_from_page(0x1c2, 0x276, 8, 0x69);
        gfx_copy_from_page(0x140, 0x276, 0xd2, 0x14c);
        return;
    }
    if (PENDING_REL(p) != 0 && menu(0x1c2, 5, 0x276, 0x48, S(UW(0xcb0)), p) == 2) {
        gfx_copy_from_page(0x1c2, 0x276, 8, 0x7e);
        gfx_copy_from_page(0x140, 0x276, 0xd2, 0x14c);
        return;
    }
    if (rel > 1 && rel < 10) {
        int choice;
        if (rel == 9 && REL(7, tgt) < 7) choice = menu(0x1bc, 5, 0x276, 0x52, S(UW(0xcba)), p);
        else if (rel < 7) choice = menu(400, 5, 0x276, 0x68, S(UW(0xcb6)), p);
        else choice = menu(400, 5, 0x276, 0x68, S(UW(0xcb8)), p);
        if (choice == 1 && REL(7, tgt) > 6) notice(0x28d4, UW(0x2526), 0x28d2);
        if (choice == 1 && F(7, p, 0) != 0) notice(0x28e5, UW(0x24d8), 0x28e3);
        if (choice == 1 && rel < 4) notice(0x28f6, UW(0x2576), 0x28f4);
        if (choice == 1 && TARGET(p) != 7 && rel > 3) {
            if (rel > 6 && MONEY(7) > 0x96) { MONEY(7) -= 0x96; MONEY(p) += 0x96; }
            if (REL(7, tgt) < 7 && F(7, p, 0) == 0 && rel - rnd(4) - 2 > 0) {
                PENDING_REL(p) = (i16)(rel + 1);
                if (wars != 0 && rel < 9) PENDING_REL(p) = (i16)(rel + 2);
            } else PENDING_REL(p) = (i16)rel;
            if (PENDING_REL(p) == 10 && POWER(7) < 8 && TURN < 10) POWER(7)++;
        } else PENDING_REL(p) = (i16)rel;
        if (choice == 2 && (rel > 4 || (rel < 5 && F(7, p, 9) == 1))) PENDING_REL(p) = (i16)(rel - 1);
    }
    if (rel > 9) {
        int choice = menu(0x1bd, 5, 0x276, 0x5f, S(UW(0xcbc)), p);
        if (choice == 1) {
            i16 aid = (i16)((double)MONEY(7) * 0.4);
            MONEY(7) -= aid; MONEY(7) += aid;     // sic: the original credits the aid back to Israel
            PENDING_REL(p) = (i16)rel;
        }
        if (choice == 2) PENDING_REL(p) = (i16)(rel - 1);
    }
    if (rel == 1 && ATWAR(p) == 0) {
        menu(0x1c2, 5, 0x276, 100, S(UW(0xcea)), p);
        PENDING_REL(p) = (i16)rel;
    }
    if (rel == 1 && ATWAR(p) != 0) {
        if (PEACE_TALKS(p) == 1) {                // they have offered peace
            if (menu(0x1ae, 5, 0x276, 0x4b, S(UW(0xcda)), p) == 1) {
                char *body = S(0xa2ba);
                strcpy(title, S(0x2905));
                strcpy(body, S(0x2916)); strcat(body, S(UW(0xd30 + p * 2))); strcat(body, S(0x295c));
                strcpy(extra, S(0x29fb));
                text_box(0x82, 100, 500, 0xc3, title, body, extra);
                if (menu(0x1ae, 5, 0x276, 0x55, S(UW(0xcf2)), 0) == 1) make_peace(7, p);
                gfx_copy_from_page(0x82, 500, 0xaf, 0x155);
                gfx_copy_from_page(0x1ae, 0x276, 8, 0x94);
            } else peace_news(7, p, 2);
            PEACE_TALKS(p) = 0;
        } else {
            PEACE_TALKS(p) = (menu(0x1c2, 5, 0x276, 0x5a, S(UW(0xcd8)), p) == 1) ? 2 : 0;
            PENDING_REL(p) = 1;
        }
    }
    gfx_copy_from_page(400, 0x276, 8, 0xd2);
    gfx_copy_from_page(0x140, 0x276, 0xd2, 0x14c);
}

void intel_screen(int p) {                        // 1000:6FA6
    int status = STATUS(p), unrest = UNREST(p);
    char *title = S(0xa1f2), *body = S(0xa2ba), *extra = S(0xa3b4);
    strcpy(title, S(0x2e58));
    strcpy(extra, S(0x2e83)); strcat(extra, S(UW(0x2a94 + status * 2))); strcat(extra, S(0x2e99));
    if (status == 10) {
        if (CONQUEROR(p) < 8) {
            if (CONQUEROR(p) >= 0) {
                strcat(extra, S(0x2eef)); strcat(extra, S(UW(0xd30 + CONQUEROR(p) * 2)));
                if (STATUS(CONQUEROR(p)) == 10) { strcat(extra, S(0x2f03)); strcat(extra, S(UW(0xd30 + CONQUEROR(CONQUEROR(p)) * 2))); }
                strcat(extra, S(0x2f09));
            }
        } else {
            strcat(extra, S(0x2ed6)); strcat(extra, S(UW(0xfae + W(0xa13e + p * 2) * 2))); strcat(extra, S(0x2eec));
        }
    } else if (unrest != 0) {
        if (unrest == 1) { strcat(extra, S(0x2e9c)); strcat(extra, S(UW(0xfae + W(0xa13e + p * 2) * 2))); }
        else { strcat(extra, S(0x2eb7)); strcat(extra, S(UW(0x2e4c + (unrest / 2) * 2))); }
        strcat(extra, S(0x2ed3));
    }
    int level = unrest + status - POWER(p) - 4;
    if (level < -2) strcpy(body, S(UW(0x2dd8)));
    else if (level < 1) strcpy(body, S(UW(0x2dda)));
    if (level > 0 && level < 10) strcpy(body, S(UW(0x2dd8 + level * 2)));
    if (level > 9 && level < 0xc) strcpy(body, S(UW(0x2dea)));
    if (level > 0xb) strcpy(body, S(UW(0x2dec)));
    if (level > 2 && level < 9 && ATWAR(p) != 0) strcpy(body, S(UW(0x2df0)));
    if (status == 9) strcpy(body, S(UW(0x2dee)));
    if (status == 10 && CONQUEROR(p) > 7) strcpy(body, S(UW(0x2df4)));
    if (status == 10 && CONQUEROR(p) < 8) strcpy(body, S(UW(0x2df2)));
    text_box(0xfa, 0x78, 0x276, 0xc3, title, body, extra);
    if (PENDING_UNREST(p) != 0 && menu(0x1b8, 5, 0x276, 0x50, S(UW(0xcb2)), p) == 2) {
        gfx_copy_from_page(0x1b8, 0x276, 8, 0x8c);
        gfx_copy_from_page(0xfa, 0x276, 0xd2, 0x155);
        return;
    }
    int choice;
    if (unrest == 0) choice = menu(0x1a9, 5, 0x276, 0x5c, S(UW(0xcc0)), p);
    else if (STATUS(p) == 10 || (unrest < 8 && status < 6)) choice = menu(0x154, 5, 0x276, 0x69, S(UW(0xcbe)), p);
    else choice = menu(0x140, 5, 0x276, 100, S(UW(0xcfa)), p);
    if (rnd(5) < 2) PENDING_UNREST(p) = (i16)unrest;      // the operation has no effect this month
    else {
        if (choice == 1 && status < 9) { MONEY(7) -= 2; if (unrest < 10) PENDING_UNREST(p) = (i16)(unrest + 1); }
        if (choice == 2 && unrest != 0) { MONEY(7) -= 2; PENDING_UNREST(p) = (i16)(unrest - 1); }
    }
    if (status < 10 && (unrest > 7 || status > 5) && choice == 3) {       // "Consider extreme measures"
        gfx_copy_from_page(0xfa, 0x276, 0xc0, 0x155);
        gfx_copy_from_page(0x140, 0x276, 8, 0xb7);
        int m = menu(400, 5, 0x276, 0x5a, S(UW(0xcfc)), p);
        if (m == 1) COVERT_ORDER(p) = 1;
        if (m == 2) COVERT_ORDER(p) = 2;
        PENDING_UNREST(p) = (i16)unrest;
    }
    gfx_copy_from_page(0xfa, 0x276, 0xc0, 0x155);
    gfx_copy_from_page(0x140, 0x276, 8, 0xb7);
}

int covert_news_posted;                           // covert_operations() queued news this month (see ai_month_begins)

void covert_operations(void) {                    // 1000:74E1 - carry out assassinations and coups
    covert_news_posted = 0;
    for (int c = 0; c < 7; c++) {
        u16 head = 0, story = 0;
        if (COVERT_ORDER(c) == 1) {
            int r = rnd(5) + STATUS(c) + W(0x91a6 + c * 2) * -5;
            if (r > 10) { STATUS(c) = 10; government_falls(c); head = UW(0x2fe6); story = UW(0x3138); }
            if (r > 6 && r < 0xb) {
                STATUS(c)++;
                if (STATUS(c) == 10) government_falls(c);
                head = UW(0x2fe8); story = UW(0x313a);
            }
            if (r < 7) { W(0xb276) += 5; W(0xb264) -= 0xf; head = UW(0x2fea); story = UW(0x313c); }
            W(0x91a6 + c * 2)++;
            if (head) { add_headline(head, c, 7); covert_news_posted = 1; }
            if (story) { add_story(story, c, 7); covert_news_posted = 1; }
        }
        if (COVERT_ORDER(c) == 2) {
            MONEY(7) -= 0x32;
            int r = UNREST(c) + STATUS(c) - 0xe - F(c, c, 0) / 200;
            if (r < 1) {
                if (r > -5) { head = UW(0x2fee); story = UW(0x3140); UNREST(c) = UNREST(c) / 2; }
            } else { head = UW(0x2fec); story = UW(0x313e); STATUS(c) = 10; government_falls(c); }
            if (r < -5) { head = UW(0x2ff0); story = UW(0x3142); UNREST(c) = 0; }
            if (head) { add_headline(head, c, 7); covert_news_posted = 1; }
            if (story) { add_story(story, c, 7); covert_news_posted = 1; }
        }
    }
}

u16 weapon_name(int n) { return UW(0x32c6 + n * 2); }       // 1000:76D4
u16 weapon_blurb(int n) { return UW(0x339e + n * 2); }      // 1000:76E4
