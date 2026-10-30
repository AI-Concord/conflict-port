// Monthly briefing: newspapers, Foreign Office summary, per-country screens and the
// policy menu, plus the text-template and bookkeeping helpers they rely on
// (original 1000:4675 - 1000:5AA4, 1000:6C39 - 1000:6FA5, 1D93:1996).
#include "conflict.h"

#define TURN       W(0xb268)
#define MONTH      W(0xb274)
#define YEAR       W(0xb272)
#define TENSION    W(0xb262)
#define WAR(a, b)  W(0x9310 + (a) * 0x10 + (b) * 2)
#define ADVICE(n)  S(UW(0x2190 + (n) * 2))                   // 1000:501C
#define PENDING_REL(c)    W(0xa0de + (c) * 2)                // relation change ordered this month
#define PENDING_UNREST(c) W(0xa0ee + (c) * 2)

void int_to_str(int v, char *out) {               // 1D93:1996
    int n = v < 0 ? -v : v, i = 0;
    do { out[i++] = (char)(n % 10 + '0'); n /= 10; } while (n > 0);
    if (v < 0) out[i++] = '-';
    out[i] = 0;
    for (char *a = out, *b = out + strlen(out) - 1; a < b; a++, b--) { char t = *a; *a = *b; *b = t; }
}

// 1000:6D35 - expand a message template. Placeholders: * # + refer to country a,
// @ _ & to country b, and % ^ ~ } { to fixed words.
void expand_template(u16 tmpl, int a, int b, char *out) {
    u16 src = tmpl; int pos = 0;
#define SUB(text, ch) if (B(src) == (ch)) { const char *t_ = (text); if (pos + (int)strlen(t_) < 380) { strcat(out, t_); pos += (int)strlen(t_); } src++; }
    while (B(src) != 0) {
        SUB(S(UW(0xd80 + a * 2)), '*')
        SUB(S(UW(0xd30 + a * 2)), '#')
        SUB(S(UW(0x10ec + a * 2)), '+')
        SUB(S(UW(0xd80 + b * 2)), '@')
        SUB(S(UW(0xd30 + b * 2)), '_')
        SUB(S(0x29fd), '%')
        SUB(S(0x2a04), '^')
        SUB(S(0x2a08), '~')
        SUB(S(0x2a0c), '}')
        SUB(S(0x2a13), '{')
        SUB(S(UW(0x10ec + b * 2)), '&')
        if (pos >= 380) break;                     // (the original could overrun its 100-byte buffer)
        out[pos++] = (char)B(src); src++;
    }
#undef SUB
    out[pos] = 0;
    W(0x91a0) = (i16)pos; UW(0x91a4) = src;
}

// The original stored headlines and stories with no bounds checks: 25 pointers and
// 1000 bytes of headline text (2500 for stories), followed in memory by the news counters
// themselves. A month with too much news overwrote those counters and the game hung on the
// next newspaper screen. Here the excess is dropped instead.
char last_news_text[400];                         // most recent expansion (see ai_month_begins)
#define MAX_NEWS_ITEMS 25
#define HEADLINE_BYTES 1000
#define STORY_BYTES    0x9c4

void add_headline(u16 tmpl, int a, int b) {       // 1000:6C39 - queue a newspaper headline
    char buf[400]; memset(buf, 0, sizeof buf);
    expand_template(tmpl, a, b, buf);
    memcpy(last_news_text, buf, sizeof last_news_text);
    int len = (int)strlen(buf); if (len == 0) len = 1;
    if (W(0xb25c) >= MAX_NEWS_ITEMS || W(0xb25e) + len + 1 > HEADLINE_BYTES) return;
    UW(0xa16e + W(0xb25c) * 2) = (u16)(W(0xb25e) + 0xae72);
    int i = 0;
    do { B(0xae72 + W(0xb25e)) = (u8)buf[i]; W(0xb25e)++; i++; } while (buf[i] != 0);
    B(0xae72 + W(0xb25e)) = 0; W(0xb25e)++;
    W(0xb25c)++;
}

void add_story(u16 tmpl, int a, int b) {          // 1000:6CB7 - queue a line for the country news pages
    char buf[400]; memset(buf, 0, sizeof buf);
    expand_template(tmpl, a, b, buf);
    memcpy(last_news_text, buf, sizeof last_news_text);
    int len = (int)strlen(buf); if (len == 0) len = 1;
    if (W(0xb25a) >= MAX_NEWS_ITEMS || W(0xb260) + len + 1 > STORY_BYTES) return;
    UW(0xa1a0 + W(0xb25a) * 2) = (u16)(W(0xb260) + 0xa4ae);
    int i = 0;
    do { B(0xa4ae + W(0xb260)) = (u8)buf[i]; W(0xb260)++; i++; } while (buf[i] != 0);
    B(0xa4ae + W(0xb260)) = 0; W(0xb260)++;
    W(0xb25a)++;
}

void retarget(int p) {                            // 1000:6E8E - pick the country p now concentrates on
    int v = REL(7, p);
    if (p == 0) {
        TARGET(0) = 7;
        if (v > 7) { if (STATUS(1) != 10) TARGET(0) = 1; if (STATUS(2) != 10) TARGET(0) = 2; }
        if (STATUS(5) != 10 && REL(5, 0) < 7) TARGET(0) = 5;
    }
    if (p == 6) {
        TARGET(6) = 7;
        if (REL(6, 2) < v && STATUS(2) != 10) TARGET(6) = 2;
        if (REL(6, 3) < v && STATUS(3) != 10) TARGET(6) = 3;
        if (REL(6, 4) < v && STATUS(4) != 10) TARGET(6) = 4;
    }
    if (p == 1 && STATUS(2) != 10) TARGET(1) = 2;
    if (p == 2) {
        if (STATUS(1) != 10) TARGET(2) = 1;
        if (STATUS(6) != 10 && REL(6, 2) < 7) TARGET(2) = 6;
    }
    if (p == 3 && REL(6, 3) < v && STATUS(6) != 10) TARGET(3) = 6;
    if (STATUS(TARGET(p)) == 10) TARGET(p) = 7;
}

void absorb_forces(int c) {                       // 1000:595D - forces facing a fallen country go home
    for (int e = 0; e < 8; e++)
        for (int k = 0; k < 9; k++) {
            if (e == 7 && k == 0) { W(0x98be) += W(0x98b0 + c * 2); W(0x98b0 + c * 2) = 0; }
            F(e, e, k) += F(e, c, k);
            F(e, c, k) = 0;
        }
    for (int e = 0; e < 8; e++) for (int k = 0; k < 9; k++) F(c, e, k) = 0;
}

void government_falls(int c) {                    // 1000:58BE
    u16 headline = 0;
    if (W(0xa12e + c * 2) != 5) headline = UW(0x2484);
    STATUS(c) = 10; POWER(c) = 0; W(0xa15e + c * 2) = 0;
    absorb_forces(c);
    for (int i = 0; i < 8; i++) {
        if (c < i) WAR(i, c) = 0;
        if (i < c) WAR(c, i) = 0;
    }
    if (headline) add_headline(headline, c, c);
    add_story(UW(0x2486), c, c);
}

void redeploy_border_troops(void) {               // 1000:5A52
    for (int c = 0; c < 7; c++) if (ATWAR(c) == 0) {
        int d = W(0x98b0 + c * 2) * 200 - F(7, c, 0);
        if (d < F(7, 7, 0)) { F(7, 7, 0) -= (i16)d; F(7, c, 0) += (i16)d; }
    }
}

// Set when the last step of world_events() reported a change of relations. In the
// original this decided what an uninitialised local of country_turn() contained the
// following month (see ai_month_begins).
int world_events_ended_with_news;

void world_events(void) {                         // 1000:56C7 - end-of-month drift in Arab relations
    int n = 0;
    world_events_ended_with_news = 0;
    if (W(0xb264) > 0x32) world_news(2);
    for (int c = 0; c < 7; c++) if (TURN > 2 && W(0x98d2 + c * 100 + (TURN - 1) * 2) != 0) n++;
    if (n != 0 && W(0xb264) < 0x32) world_news(1);
    if (TENSION < 0x98 && ((TURN - 5) % 12 == 0 || TURN == 5)) world_news(3);
    int count = TENSION < 0x9b ? 2 : 1;
    for (int j = 0; j < count; j++) {
        int a, b;
        do {
            do { a = rnd(7); b = rnd(7) - 1; } while (STATUS(a) == 10);
        } while (STATUS(b) == 10 || a <= b);
        world_events_ended_with_news = 0;
        int old = REL(a, b);
        if (old == 2) REL(a, b) = 1;
        if (old == 5) {
            int e = TARGET(b);
            REL(a, b) = (TARGET(a) == e || WAR(a, e) != 0) ? 6 : 4;
        }
        if (STATUS(a) == 9) government_falls(a);
        if (STATUS(b) == 9) government_falls(b);
        if (STATUS(a) == 10 || STATUS(b) == 10) return;
        if (old == 6) REL(a, b) = 7;
        if (old != REL(a, b)) { relations_news(a, b); world_events_ended_with_news = 1; }
    }
}

void recolor_fallen(void) {                       // 1000:4F69 - black out fallen countries on the regional map
    if (STATUS(0) == 10) gfx_recolor(0, 100, 0xa5, 0x103, 10, 0);
    if (STATUS(4) == 10) gfx_recolor(0x32, 0x19, 200, 0x50, 10, 0);
    if (STATUS(2) == 10) gfx_recolor(0xfa, 0, 300, 200, 10, 0);
    if (STATUS(3) == 10) gfx_recolor(0x65, 0x46, 0xfa, 0x96, 2, 0);
    if (STATUS(6) == 10) gfx_recolor(0x65, 0, 0xfa, 0x69, 0xe, 0);
}

static void set_advice_from_template(u16 tmpl, int a, int b) {
    char buf[400]; memset(buf, 0, sizeof buf);
    expand_template(tmpl, a, b, buf);
    strcpy(S(0xa2ba), buf);
}

void foreign_office_comment(void) {               // 1000:502C - choose the summary line for the news screen
    char *msg = S(0xa2ba);
    u16 t = 0;
    strcpy(msg, S(0x240e));
    if (TURN == 1) strcpy(msg, ADVICE(8));
    if (TENSION < 0x9c && TENSION > 0x96) strcpy(msg, ADVICE(1));
    if (W(0xb264) > 0x32) strcpy(msg, ADVICE(0));
    if (TENSION < 0x97 && TENSION > 0x8c) strcpy(msg, ADVICE(0xb));
    if (W(0xb266) == 6 && W(0xb26a) == 0) strcpy(msg, ADVICE(9));
    if (W(0xb26e) != 7) strcpy(msg, ADVICE(6));
    for (int a = 1; a < 7; a++)
        for (int b = 0; b < a; b++) {
            if (STATUS(a) == 10 || STATUS(b) == 10) continue;
            if (REL(a, b) == 3 && a != 1 && (REL(7, a) < 5 || REL(7, b) < 5)) t = UW(0x23fa);
            if (REL(a, b) == 8 && F(7, a, 9) == 1 && F(7, b, 9) == 1) t = UW(0x23fc);
            if (REL(a, b) == 10 && (REL(7, a) == 3 || REL(7, b) == 3)) t = UW(0x23fe);
            if (t != 0) { set_advice_from_template(t, a, b); t = 0; }
        }
    for (int c = 0; c < 7; c++) {
        if (STATUS(c) == 10) continue;
        if (STATUS(c) > 6 && UNREST(c) < 10 && F(7, c, 9) == 1 && PENDING_UNREST(c) == 0) t = UW(0x2400);
        if (F(7, c, 0) != 0 && REL(7, c) < 4 && REL(7, c) > 1) t = UW(0x2402);
        if (TURN != 1 && REL(7, c) == 7 && PENDING_REL(c) == 0 && PENDING_UNREST(c) == 0) t = UW(0x2404);
        if (PENDING_REL(c) == 0 && REL(7, c) == 2) t = UW(0x2406);
        if (ATWAR(c) == 0 && PENDING_REL(c) == 0 && REL(7, c) == 1) t = UW(0x2408);
        if (t != 0) { set_advice_from_template(t, c, 7); t = 0; }
    }
    if (W(0xb264) > 0x22 && W(0xb264) < 0x32 && W(0xb2e6) == 0) { strcpy(msg, ADVICE(10)); W(0xb2e6) = 1; }
    if (W(0xb264) > 0x31 && W(0xb2e6) != 2) { strcpy(msg, ADVICE(0xd)); W(0xb2e6) = 2; }
    if (MONTH == 6 && TENSION < 0x96) strcpy(msg, ADVICE(7));
    for (int c = 0; c < 7; c++) {
        if (W(0xa15e + c * 2) == 1 && PENDING_REL(c) == 0 && F(7, c, 9) == 1) t = UW(0x240a);
        if (REL(7, c) < 4 && NUKE_STAGE(7) < 10 && NUKE_STAGE(c) == 10) t = UW(0x240c);
        if (t != 0) { set_advice_from_template(t, c, 7); t = 0; }
    }
}

void country_screen(int c) {                      // 1000:549B - a country's map, news and the affairs menu
    int done = 0;
    gfx_draw_map(W(0x10fe + c * 2));
    while (!done) {
        if (c == 1 || c == 2) map_overlay_libya();
        if (c == 0 || c == 5) map_overlay_egypt();
        if (c == 3 || c == 4) map_overlay_israel();
        if (c == 6) map_overlay_syria();
        char *title = S(0xa1f2), *foot = S(0xa2ba), *news = S(0xa3b4);
        strcpy(title, S(0x241c)); strcat(title, S(UW(0xd80 + c * 2))); strcat(title, S(0x241d));
        strcpy(foot, S(0x242c)); strcat(foot, S(UW(0xd80 + c * 2))); strcat(foot, S(0x242e));
        strcat(foot, S(UW(0x1098 + POWER(c) * 2))); strcat(foot, S(0x243a));
        strcpy(news, S(UW(0xa1e2 + c * 2)));
        int len = (int)strlen(news);
        if (len > 200) strcpy(foot, S(0x243d));
        if (len != 0 && len < 0x5c) text_box(0x140, 0x82, 0x276, 0xc3, title, news, foot);
        if (len > 0x5b) text_box(0x122, 100, 0x276, 0xc3, title, news, foot);
        W(0xb270) = 2;
        int choice = menu(0x1a9, 5, 0x276, 0x4c, S(UW(0xcae)), c);
        W(0xb270) = 0;
        if (choice != 3) {
            gfx_copy_from_page(0x122, 0x276, 0xaf, 0x155);
            gfx_copy_from_page(0x1a9, 0x276, 8, 0x85);
        }
        if (choice == 1) diplomacy_screen(c);
        if (choice == 2) intel_screen(c);
        if (choice == 3) done = 1;
    }
}

// The original tests this local before assigning it; see ai.c for the same pattern.
static i16 briefing_choice_slot;
i16 *briefing_residue_choice = &briefing_choice_slot;

void monthly_briefing(void) {                     // 1000:4675
    char year[16], money[16], spec[200];
    int opt[4] = {0, 0, 0, 0};
#define choice (*briefing_residue_choice)
    if (W(0xb278) == 0) {                         // skipped right after loading a saved game
        MONEY(7) = (i16)((double)MONEY(7) + (double)GNP(7) * ((double)(float)(double)BUDGET_PCT(7) / 100.0));
        {
            int a = UNREST(7), b = STATUS(7), c = POWER(7), d = W(0x98c0), e = W(0xb266);
            if (rnd(10) < a + b - c - d * e) {
                STATUS(7)++;
                if (STATUS(7) > 9) game_over(0, 0, 7, 0, 0);
                unrest_news(7);
            }
        }
        if (TURN == 1) { YEAR = 0x7cd; MONTH = 0; W(0xb2e6) = 0; }
        MONTH++;
        if (MONTH == 0xd) { YEAR++; MONTH = 1; }
        for (int c = 0; c < 8; c++) { W(0xa12e + c * 2) = 0; PENDING_REL(c) = 0; PENDING_UNREST(c) = 0; W(0xb2d6 + c * 2) = 0; }
        TENSION = 0;
        for (int a = 1; a < 8; a++)
            for (int b = 0; b < a; b++) {
                if (STATUS(a) < 10 && STATUS(b) < 10) {
                    TENSION += REL(a, b);
                    if (WAR(a, b) != 0) TENSION -= WAR(a, b);
                } else TENSION += 5;
            }
        if (TURN > 2 && MONTH == 1) yearly_review(YEAR);
        if (MONTH == 7 && TENSION < 0x96) summit();
        if (STATUS(6) > 9 && STATUS(0) > 9 && STATUS(4) > 9 && STATUS(3) > 9) game_over(1, 1, 99, 0, 0);
    }
    W(0xb278) = 0;
    int_to_str(YEAR, year);
    if (W(0xb25c) != 0) {
        draw_newspapers();
        char *s = S(0xa2ba);
        strcpy(s, S(month_name(MONTH))); strcat(s, S(0x1b5c)); strcat(s, year); strcat(s, S(0x1b5e));
        W(0xb270) = 5;
        menu(0x1d6, 5, 0x276, 0x2a, s, 0);
        W(0xb270) = 0;
    }
    gfx_draw_map(0);
    recolor_fallen();
    for (int i = 0; i < 1000; i++) B(0xae72 + i) = 0;
    W(0xb25c) = 0; W(0xb25e) = 0;
    while (choice != 8) {                         // Foreign Office: pick a country or "Make Policy"
        int_to_str(MONEY(7), money);
        char *title = S(0xa1f2), *extra = S(0xa3b4);
        strcpy(title, S(month_name(MONTH))); strcat(title, S(0x1b81)); strcat(title, year); strcat(title, S(0x1b83));
        foreign_office_comment();
        strcpy(extra, S(0x1b93));
        strcat(extra, S(UW(0x1098 + POWER(7) * 2)));
        strcat(extra, S(0x1bb3));
        if (TENSION < 0x79) strcat(extra, S(0x1bcd));
        else strcat(extra, S(UW(0x1032 + ((TENSION - 0x78) / 5) * 2)));
        strcat(extra, S(0x1bda));
        text_box(0x9b, 0x7d, 0x203, 0xbe, title, S(0xa2ba), extra);
        W(0xb270) = 1;
        choice = (i16)menu(0x20d, 5, 0x276, 0xaa, S(UW(0xca4)), 0);
        W(0xb270) = 0;
        if (choice < 8) country_screen(choice - 1);
        gfx_draw_map(0);
        recolor_fallen();
    }
    for (int c = 0; c < 8; c++) {
        int v = PENDING_REL(c);
        if (v != 0 && v != REL(7, c)) { REL(7, c) = (i16)v; relations_changed(7, c); }
        v = PENDING_UNREST(c);
        if (v != 0) UNREST(c) = (i16)v;
    }
    for (;;) {                                    // National Security Office: the policy menu
        char *advice = S(0xa2ba), *title = S(0xa1f2), *extra = S(0xa3b4);
        strcpy(advice, S(0x1bdd));
        if (TURN > 2 && (NUKE_STAGE(7) < NUKE_STAGE(0) || NUKE_STAGE(7) < NUKE_STAGE(6))) strcpy(advice, ADVICE(0x11));
        if (TURN > 5 && W(0xb264) < 10 && REL(7, 0) > 2 && REL(7, 6) > 2 && REL(7, 4) > 2 && REL(7, 3) > 2) strcpy(advice, ADVICE(0x14));
        if (MONTH == 7 && W(0xb26c) == 1) strcpy(advice, ADVICE(0x12));
        if (W(0xb26a) == 0) {
            if (STATUS(7) > 3) strcpy(advice, ADVICE(0xc));
            if (STATUS(7) > 5) strcpy(advice, ADVICE(3));
            if (STATUS(7) > 7) strcpy(advice, ADVICE(4));
            if (STATUS(7) > 3 && MONTH == 5) strcpy(advice, ADVICE(0xe));
        }
        if (ATWAR(0) == 3) strcpy(advice, ADVICE(0x15));
        if (ATWAR(6) == 2) strcpy(advice, ADVICE(0x16));
        if (W(0xb26a) == 1) {
            if (STATUS(7) == 6) strcpy(advice, ADVICE(0xf));
            if (STATUS(7) > 6) strcpy(advice, ADVICE(0x10));
        }
        if (W(0xb2e6) == 1 && W(0xb264) > 0x32) strcpy(advice, ADVICE(0x13));
        if (W(0xb26e) != 7) strcpy(advice, ADVICE(2));
        for (int c = 0; c < 7; c++) if (F(c, 7, 0) != 0 && F(7, c, 0) == 0) strcpy(advice, ADVICE(5));
        int_to_str(MONEY(7), money);
        int_to_str(YEAR, year);
        strcpy(spec, S(UW(0xca6)));
        strcpy(title, S(month_name(MONTH))); strcat(title, S(0x1c09)); strcat(title, year); strcat(title, S(0x1c0b));
        strcpy(extra, S(0x1c25));
        strcpy(extra, S(0x1c3f)); strcat(extra, money); strcat(extra, S(0x1c59));
        text_box(300, 0x87, 0x276, 0xbe, title, advice, extra);
        int n = 0;
        if (NUKE_STAGE(7) != 10) opt[n++] = 0;
        if (W(0xb26a) == 0) opt[n++] = 1;
        opt[n++] = 2;
        for (int i = 0; i < n; i++) strcat(spec, S(UW(0x1b56 + opt[i] * 2)));
        W(0xb270) = 3;
        choice = (i16)menu(400, 8, 0x276, 0x7b, spec, 0);
        W(0xb270) = 0;
        gfx_copy_from_page(300, 0x276, 0xec, 0x14c);
        gfx_copy_from_page(400, 0x276, 0xe, 0xd7);
        if (STATUS(2) == 10) recolor_fallen();
        if (choice == 1) arms_menu();
        if (choice == 2) strategic_menu();
        if (choice == 3) forces_review();
        if (choice - 4 >= 0) {
            if (opt[choice - 4] == 0) nuclear_policy();
            if (opt[choice - 4] == 1) policing_menu();
            if (opt[choice - 4] == 2) return;
        }
        gfx_draw_map(0);
        recolor_fallen();
    }
#undef choice
}
