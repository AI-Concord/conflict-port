// Monthly hostile acts short of war: Arab raids and sabotage against their targets and
// the Israeli strikes ordered by the player (original 1B92:0007 and 1B92:0714).
#include "conflict.h"

#define ACTION(c)   W(0xa12e + (c) * 2)
#define CONDEMNED   W(0xb264)
#define EVENT       W(0xb26e)                     // last attack on Israel: attacker + 100 * kind (7 = none)
#define SHIELD      W(0x989c)                     // Israel's early-warning aircraft

static void lose_air_wing(void) { if (F(7, 7, 2) < 3) F(7, 7, 2) = 0; else F(7, 7, 2) -= 3; }

void israeli_strikes(void) {                      // 1B92:0714
    for (int c = 0; c < 7; c++) {
        int r = rnd(10);
        if (ACTION(c) == 1 && F(7, 7, 2) != 0) {  // strike on industry
            if (F(7, 7, 2) < 3) r -= 5;
            if (r < 2) { lose_air_wing(); strike_news(2, 1, 7, c); }
            if (r > 1 && r < 0xb) {
                CONDEMNED += 10;
                GNP(c) -= (i16)r;
                if ((c == 0 || c == 4 || c == 3) && GNP(c) < 0x1e) GNP(c) = 0x1e;
                if (c == 6 && GNP(6) < 0x32) GNP(6) = 0x32;
                if (r < 6) strike_news(2, 3, 7, c); else strike_news(2, 4, 7, c);
                if (REL(7, c) > 1) REL(7, c)--;
            }
        }
        if (ACTION(c) == 2 && F(7, 7, 2) != 0) {  // strike on military targets
            if (F(7, 7, 2) < 3) r -= 5;
            if (r < 3) { lose_air_wing(); strike_news(1, 1, 7, c); }
            if (r > 2 && r < 6) {
                CONDEMNED += 10;
                if (r == 4 && F(c, c, 3) > 2) F(c, c, 3) -= 2;
                if (r == 5 && F(c, c, 2) > 1) F(c, c, 2) -= 1;
                if (REL(7, c) > 1) REL(7, c)--;
                strike_news(1, 3, 7, c);
            }
            if (r > 5 && r < 0xb) {
                CONDEMNED += 0xc;
                if (r > 5 && r < 10 && F(c, c, 1) > 0) F(c, c, 1) = F(c, c, 1) / 2;
                if (r == 10 && F(c, c, 2) < 6) F(c, c, 2) = 0; else F(c, c, 2) -= 3;
                if (POWER(7) < 0xb) POWER(7)++;
                if (REL(7, c) > 1) REL(7, c)--;
                strike_news(1, 4, 7, c);
            }
        }
        if (ACTION(c) == 3 && F(7, 7, 2) != 0) {  // strike on civilian targets
            W(0x9300 + c * 2) -= (i16)r;
            if (STATUS(c) < 9) STATUS(c)++;
            if (r < 3) { lose_air_wing(); strike_news(3, 1, 7, c); }
            if (r > 2) {
                if (REL(7, c) > 1) REL(7, c)--;
                CONDEMNED += 0x1e;
                if (r < 6) strike_news(3, 3, 7, c); else strike_news(3, 4, 7, c);
            }
        }
        if (ACTION(c) == 4 && F(7, 7, 2) != 0) {  // strike on a nuclear facility
            if (r < 3) { lose_air_wing(); strike_news(4, 1, 7, c); }
            else {
                if (NUKE_STAGE(c) < 10) NUKE_STAGE(c) -= 3;
                if (POWER(7) < 10) POWER(7)++;
                strike_news(4, 4, 7, c);
                if (REL(7, c) > 1) REL(7, c)--;
            }
        }
    }
    for (int c = 0; c < 7; c++) ACTION(c) = 0;
}

void hostile_acts(void) {                         // 1B92:0007 - countries act in order of prestige
    int order[16], n = 0;
    EVENT = 7;
    for (int p = 10; p > 0; p--) for (int c = 0; c < 8; c++) if (POWER(c) == p) order[n++] = c;
    for (int j = 0; j < 8 && j < n; j++) {
        if (order[j] == 7) israeli_strikes();
        int a = order[j];
        if (a == 7 || a < 0 || a >= 8 || STATUS(a) == 10) continue;
        int kind = rnd(8);
        int t = TARGET(a);
        int rel = (a < t) ? REL(t, a) : REL(a, t);
        if (F(a, a, 2) > 2 && (rel == 3 || rel == 2) && STATUS(a) != 10 && STATUS(t) != 10 && F(a, t, 9) != 0 && kind < 3) {
            int r = rnd(10);                      // air raid on military targets
            if (SHIELD != 0 && t == 7) r -= 2;
            if (F(t, t, 3) > 0 && r < 2) { F(a, a, 2) -= 3; strike_news(1, 1, a, t); }
            if (r > 1 && r < 5) strike_news(1, 2, a, t);
            if (r > 4 && r < 9) {
                if ((r == 5 || r == 6) && F(t, t, 1) > 6) F(t, t, 1) -= 6;
                if (r == 7 && F(t, t, 3) > 2) F(t, t, 3) -= 2;
                if (r == 8 && F(t, t, 2) > 1) F(t, t, 2) -= 1;
                strike_news(1, 3, a, t);
                if (t == 7) EVENT = (i16)a;
            }
            if (r > 8 && r < 0xb) {
                if (r == 9 && F(t, t, 1) > 0) F(t, t, 1) = F(t, t, 1) / 2;
                if (r == 10 && F(t, t, 2) < 6) F(t, t, 2) = 0; else F(t, t, 2) -= 3;
                if (POWER(a) < 0xb) POWER(a)++;
                rel--;
                strike_news(1, 4, a, t);
                if (t == 7) EVENT = (i16)(a + 100);
            }
        }
        if (F(a, t, 9) != 0 && kind == 3 && F(a, a, 2) > 2 && STATUS(a) != 10 && STATUS(t) != 10 && (rel == 2 || rel == 3)) {
            int r = rnd(10);                      // air raid on industry
            if (SHIELD != 0 && t == 7) r -= 1;
            if (r < 4) { F(a, a, 2) -= 3; strike_news(2, 1, a, t); }
            else {
                GNP(t) += (i16)(r * -0x32);
                if (r > 7 && (t == 1 || t == 2)) GNP(t) -= 0x96;
                if ((t == 6 || t == 4 || t == 3) && GNP(t) < 0x32) GNP(t) = 0x32;
                if ((t == 7 || t == 0 || t == 1 || t == 2) && GNP(t) < 500) GNP(t) = 500;
                rel--;
                if (r < 8) strike_news(2, 3, a, t); else strike_news(2, 4, a, t);
                if (t == 7) EVENT = (i16)(a + 200);
            }
        }
        if (F(a, t, 9) != 0 && kind == 4 && F(a, a, 2) > 2 && STATUS(a) != 10 && STATUS(t) != 10 && (rel == 2 || rel == 3) && POWER(a) < 4) {
            int r = rnd(10);                      // attack on civilians
            if (SHIELD != 0 && t == 7) r -= 2;
            if (r > 6 && STATUS(t) < 9) STATUS(t)++;
            if (r > 0 && r < 3) strike_news(3, 1, a, t);
            if (r > 2 && r < 7) strike_news(3, 2, a, t);
            if (r > 6 && r < 9) strike_news(3, 3, a, t);
            if (r == 10) strike_news(3, 4, a, t);
            if (r > 6) rel--;
            if (t == 7 && r > 6) EVENT = (i16)(a + 300);
        }
        if (rel < 1) rel = 1;
        if (a < t) REL(t, a) = (i16)rel; else REL(a, t) = (i16)rel;
    }
}
