// Monthly war simulation: air, helicopter and ground battles on every active front,
// peace feelers and peace settlements (original 1B92:0BD1 - 1B92:2016).
#include <math.h>
#include "conflict.h"

#define WAR(a, b)       W(0x9310 + (a) * 0x10 + (b) * 2)     // months of war between a and b (a > b)
#define FRONT_LINE(c)   W(0x98a0 + (c) * 2)
#define BRIGADES(c)     W(0x98b0 + (c) * 2)
#define POOL            W(0x98be)
#define PEACE_TALKS(c)  W(0xa15e + (c) * 2)
#define RESERVES(c)     W(0x9300 + (c) * 2)
#define WEARINESS(c)    W(0x98c2 + (c) * 2)

// The original's software floating point returned 0 for 0/0 and infinity for x/0.
static double zdiv(double a, double b) { if (b == 0.0) return a == 0.0 ? 0.0 : (a > 0 ? INFINITY : -INFINITY); return a / b; }
static int sign(int v) { return (v > 0) - (v < 0); }
static void abort_key(void) { if (gfx_getkey() == 0x45) game_longjmp(999); }

static void air_battle(int sorties_a, int sorties_b, int a, int b) {   // 1B92:1993
    i16 men_a = F(a, b, 0), tanks_a = F(a, b, 1), guns_a = F(a, b, 4);
    i16 men_b = F(b, a, 0), tanks_b = F(b, a, 1), guns_b = F(b, a, 4);
    int flak_a = (i16)((i16)(F(a, b, 5) * 100) / (i16)(men_a + 1)) + men_a / 0x32;
    int flak_b = (i16)((i16)(F(b, a, 5) * 100) / (i16)(men_b + 1)) + men_b / 0x32;
    for (int i = 0; i < sorties_b; i++) {         // b's aircraft attack a's ground forces
        int x = rnd(5) + 2;
        if (tanks_a <= x && x < guns_a) guns_a -= (i16)x;
        if (x < tanks_a) tanks_a -= (i16)x;
        int y = rnd(3) + 1;
        if (y < men_a) men_a -= (i16)y;
        if (rnd(100) < flak_a) F(b, a, 3)--;
    }
    for (int i = 0; i < sorties_a; i++) {
        int x = rnd(5) + 2;
        if (tanks_b <= x && x < guns_b) guns_b -= (i16)x;
        if (x < tanks_b) tanks_b -= (i16)x;
        int y = rnd(3) + 1;
        if (y < men_b) men_b -= (i16)y;
        if (rnd(100) < flak_b) F(a, b, 3)--;
    }
    F(a, b, 0) = men_a; F(a, b, 1) = tanks_a;
    if (F(a, b, 1) < 1) F(a, b, 1) = 0;
    if (F(a, b, 0) < 1) F(a, b, 0) = 0;
    F(a, b, 4) = guns_a;
    F(b, a, 0) = men_b; F(b, a, 1) = tanks_b;
    if (F(b, a, 1) < 1) F(b, a, 1) = 0;
    if (F(b, a, 0) < 1) F(b, a, 0) = 0;
    F(b, a, 4) = guns_b;
}

static void helicopter_battle(int a, int b) {     // 1B92:16C1
    i16 heli_b = F(b, a, 7), heli_a = F(a, b, 7), special = F(7, b, 6);
    i16 men_a = F(a, b, 0), men_b = F(b, a, 0), tanks_a = F(a, b, 1), tanks_b = F(b, a, 1);
    int flak_a = (i16)((i16)(F(a, b, 5) * 100) / (i16)(men_a + 1)) + men_a / 0x32;
    int flak_b = (i16)((i16)(F(b, a, 5) * 100) / (i16)(men_b + 1)) + men_b / 0x32;
    if ((i16)(heli_a * 4) <= tanks_b) tanks_b -= (i16)(heli_a * 4);
    if ((i16)(heli_b * 4) <= tanks_a) tanks_a -= (i16)(heli_b * 4);
    for (int i = 0, n = heli_a; i < n; i++) { int r = rnd(100); if ((double)flak_b * 1.5 > (double)r) heli_a--; }
    for (int i = 0, n = heli_b; i < n; i++) { int r = rnd(100); if ((double)flak_a * 1.5 > (double)r) heli_b--; }
    for (int i = 0, n = special; i < n; i++) {    // Israeli electronic-warfare aircraft
        int r = rnd(100);
        if (r < 0x1e && F(b, 7, 5) > 10) F(b, 7, 5) -= 3;
        if (r > 0x1d && r < 0x3c && heli_b > 3) heli_b -= 3;
        if (r >= 0x3c && tanks_b > 6) tanks_b -= 8;
        if ((double)flak_b * 1.5 > (double)r) special--;
    }
    F(a, b, 1) = tanks_a; if (F(a, b, 1) < 1) F(a, b, 1) = 0;
    F(b, a, 1) = tanks_b; if (F(b, a, 1) < 1) F(b, a, 1) = 0;
    F(a, b, 7) = heli_a;
    F(b, a, 7) = heli_b;
    F(7, b, 6) = special;
}

static void ground_battle(int a, int b) {         // 1B92:1C4D
    i16 men_a = F(a, b, 0), tanks_a = F(a, b, 1), guns_a = F(a, b, 4);
    i16 men_b = F(b, a, 0), tanks_b = F(b, a, 1), guns_b = F(b, a, 4);
    for (int round = 0; round < 3; round++) {
        i16 x = tanks_a == 0 ? 0 : (i16)(rnd(tanks_a) / 7);
        i16 y = tanks_b == 0 ? 0 : (i16)(rnd(tanks_b) / 7);
        if (round == 0) {                         // tank duel
            if (tanks_b < x && x <= guns_b) guns_b -= x;
            if (tanks_b < x) tanks_b = 0; else tanks_b -= x;
            if (tanks_a < y && y <= guns_a) guns_a -= y;
            if (tanks_a < y) tanks_a = 0; else tanks_a -= y;
        } else {                                  // tanks against infantry
            if (tanks_a == 0) men_a -= y >> 3;
            men_a -= y >> 3;
            if (tanks_b == 0) men_b -= x >> 3;
            men_b -= x >> 3;
        }
        x = guns_a == 0 ? 0 : (i16)(rnd(guns_a) >> 2);
        y = guns_b == 0 ? 0 : (i16)(rnd(guns_b) >> 2);
        if (y < tanks_a) tanks_a -= y;
        if (x < tanks_b) tanks_b -= x;
        if (men_a / 100 <= guns_b) guns_b -= men_a / 100;
        if (men_b / 100 <= guns_a) guns_a -= men_b / 100;
        int d = STATUS(a) - WEARINESS(a);
        x = (i16)(d < 2 ? rnd(men_a) : rnd(men_a / d));
        d = STATUS(b) - WEARINESS(b);
        y = (i16)(d < 2 ? rnd(men_b) : rnd(men_b / d));
        int k = (y - x) / 10;
        if (k > 0) men_a -= (i16)k;
        if (k < 0) men_b += (i16)k;
        if (men_b < 1) men_b = 0;
        if (men_a < 1) men_a = 0;
    }
    for (int i = 0, n = tanks_b; i < n; i++) { if (rnd(0x14) < 4) tanks_b--; abort_key(); }    // breakdowns
    for (int i = 0, n = tanks_a; i < n; i++) { if (rnd(0x14) < 4) tanks_a--; abort_key(); }
    F(a, b, 0) = men_a; F(a, b, 1) = tanks_a < 1 ? 0 : tanks_a; F(a, b, 4) = guns_a;
    F(b, a, 0) = men_b; F(b, a, 1) = tanks_b < 1 ? 0 : tanks_b; F(b, a, 4) = guns_b;
}

static void consider_peace(int p) {               // 1B92:12ED - does p accept Israel's ceasefire offer?
    int score = FRONT_LINE(p) > 2 ? 2 : 0;
    for (int i = 0; i < 7; i++) if (F(p, i, 9) == 1 && STATUS(i) != 10) {
        if (i < p && (REL(p, i) == 1 || REL(p, i) == 2)) score++;
        if (p < i && (REL(i, p) == 1 || REL(i, p) == 2)) score++;
    }
    if (STATUS(p) > 2 && STATUS(p) < 8) score++;
    if ((double)F(p, 7, 0) * 1.5 < (double)F(7, p, 0)) score += 2;
    if (score > 2) PEACE_TALKS(p) = 1;
}

void make_peace(int a, int b) {                   // 1B92:141B
    WAR(a, b) = 0;
    REL(a, b) = 5;
    if (a == 7) {
        if (FRONT_LINE(b) > 0) { if (POWER(7) < 10) POWER(7)++; if (STATUS(b) < 9) STATUS(b)++; }
        if (FRONT_LINE(b) < 0) { if (POWER(b) < 10) POWER(b)++; if (STATUS(7) < 9) STATUS(7)++; }
        PEACE_TALKS(7) = 0;
        retarget(b);
        for (int k = 0; k < 9; k++) {
            F(7, 7, k) += F(7, b, k); F(b, b, k) += F(b, 7, k);
            F(7, b, k) = 0; F(b, 7, k) = 0;
        }
        POOL += BRIGADES(b); BRIGADES(b) = 0;
    } else {
        if (F(b, a, 0) < F(a, b, 0)) { if (POWER(a) < 10) POWER(a)++; STATUS(a) = 0; }
        if (F(a, b, 0) < F(b, a, 0)) { if (POWER(b) < 10) POWER(b)++; STATUS(b) = 0; }
        retarget(a); retarget(b);
        for (int k = 0; k < 9; k++) {
            F(a, a, k) += F(a, b, k); F(b, b, k) += F(b, a, k);
            F(a, b, k) = 0; F(b, a, k) = 0;
        }
    }
    peace_news(a, b, 1);
}

void war_month(void) {                            // 1B92:0BD1 - fight every pair of countries at war
    int fought = 0;
    for (int a = 1; a < 8; a++)
        for (int b = 0; b < a; b++) {
            if (!(REL(a, b) == 1 && STATUS(a) < 10 && STATUS(b) < 10)) continue;
            if (WAR(a, b) != 0 || a != 7) commit_forces(a, b);
            if (PEACE_TALKS(b) == 2) consider_peace(b);
            thinking_dot();
            i16 men_a = F(a, b, 0), tanks_a = F(a, b, 1), men_b = F(b, a, 0), tanks_b = F(b, a, 1);
            if (F(a, b, 3) > 5) F(a, b, 3) -= (i16)rnd(5);
            int air_a = F(a, b, 3);
            if (F(b, a, 3) < air_a) air_a -= F(b, a, 3); else air_a = 0;
            if (F(b, a, 3) > 5) F(b, a, 3) -= (i16)rnd(5);
            int air_b = F(b, a, 3);
            if (F(a, b, 3) < air_b) air_b -= F(a, b, 3); else air_b = 0;
            abort_key();
            if (air_a != 0 || air_b != 0) { fought++; air_battle(air_a, air_b, a, b); }
            if (F(a, b, 6) != 0 || F(b, a, 6) != 0 || F(a, b, 7) != 0 || F(b, a, 7) != 0) { fought++; helicopter_battle(a, b); }
            if (tanks_a != 0 || tanks_b != 0 || F(a, b, 4) != 0 || F(b, a, 4) != 0 || men_a != 0 || men_b != 0) { fought++; ground_battle(a, b); }
            if (fought != 0) { WAR(a, b)++; consider_nuclear_strike(a, b); }
            i16 lost_a = (i16)(men_a - F(a, b, 0)), lost_ta = (i16)(tanks_a - F(a, b, 1));
            i16 lost_b = (i16)(men_b - F(b, a, 0)), lost_tb = (i16)(tanks_b - F(b, a, 1));
            if (men_a != 0 && zdiv((double)lost_b, (double)men_b) > zdiv((double)lost_a, (double)men_a)) battle_news(b, a, 0, lost_b);
            else if (men_b != 0) battle_news(a, b, 0, lost_a);
            if (tanks_b != 0 && zdiv((double)lost_ta, (double)tanks_a) > zdiv((double)lost_tb, (double)tanks_b)) battle_news(a, b, 1, lost_ta);
            else if (tanks_a != 0) battle_news(b, a, 1, lost_tb);
            if (F(a, b, 0) < (i16)(lost_a * 2) && STATUS(a) < 10) STATUS(a)++;
            if (F(b, a, 0) < (i16)(lost_b * 2) && STATUS(b) < 10) STATUS(b)++;
            if (a == 7 && fought != 0 && WAR(7, b) > 1) {      // move the front line on Israel's borders
                if (F(7, b, 1) < 1 || F(b, 7, 1) < 1) {
                    if (F(7, b, 1) == 0) { FRONT_LINE(b)--; battle_news(7, b, 2, FRONT_LINE(b)); }
                    if (F(b, 7, 1) == 0) { FRONT_LINE(b)++; battle_news(b, 7, 2, FRONT_LINE(b)); }
                } else FRONT_LINE(b) += (i16)sign(sign(lost_b - lost_a) + sign(lost_tb - lost_ta));
            }
            if (WAR(a, b) == 6) { WEARINESS(a)--; WEARINESS(b)--; }
            if (a == 7 && men_a < 1 && men_b != 0) FRONT_LINE(b) = -5;
            resolve_war(8, 8, b, a, fought);
            if (lost_a > 0 && lost_a <= RESERVES(a)) { F(a, a, 0) += lost_a; RESERVES(a) -= lost_a; }
            if (lost_b > 0 && lost_b <= RESERVES(b)) { F(b, b, 0) += lost_b; RESERVES(b) -= lost_b; }
        }
}
