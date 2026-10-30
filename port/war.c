// War bookkeeping: committing forces to a front, nuclear escalation checks and conquest
// (original 1000:5AA5 - 1000:61AF).
#include "conflict.h"

#define WAR(a, b) W(0x9310 + (a) * 0x10 + (b) * 2)

void commit_forces(int a, int b) {                // 1000:5AA5 - a and b send their armies to the common front
    int pct = 10;
    if (a == 7) {
        int d = W(0x98b0 + b * 2) * 200 - F(7, b, 0);
        if (d <= F(7, 7, 0)) { F(7, b, 0) += (i16)d; F(7, 7, 0) -= (i16)d; }
        if (F(7, 7, 0) < d) { F(7, b, 0) += F(7, 7, 0); F(7, 7, 0) = 0; }
    }
    for (int k = 0; k < 9; k++) if (k != 2) {
        if (a != 7 && a != 6 && STATUS(a) < 8) { F(a, b, k) += F(a, a, k); F(a, a, k) = 0; }
        if (b != 0 && b != 6 && STATUS(b) < 8) { F(b, a, k) += F(b, b, k); F(b, b, k) = 0; }
    }
    if (b == 0 && STATUS(0) < 8) {                // Egypt keeps a reserve unless safe on its Libyan side
        pct = (a == 7 && REL(5, 0) > 3 && STATUS(5) != 10) ? 10 : 8;
        if (a == 5) pct = 5;
    }
    if ((a == 6 || b == 6) && STATUS(6) < 8) {    // Syria likewise
        int o = (a != 6) ? a : b;
        if (o == 7 && REL(6, 3) > 3 && REL(6, 2) > 3) pct = 9;
        if (o == 7 && REL(6, 2) < 4) pct = 6;
        if (o == 7 && REL(6, 2) < 4) pct = 8;
        if (o == 3) pct = 3;
        if (o == 4) pct = 2;
        if (o == 2 && REL(7, 6) > 3) pct = 9;
        if (o == 2 && REL(7, 6) < 4) pct = 7;
    }
    for (int k = 0; k < 9; k++) if (k != 2) {
        if (a == 6) {
            i16 v = F(a, a, k), n = (i16)((i16)(pct * v) / 10);
            if (n < v) { F(a, b, k) += F(a, a, k); F(a, a, k) = 0; }
            else { F(a, b, k) += n; F(a, a, k) -= n; }
        }
        if (b == 0 || b == 6) {
            i16 v = F(b, b, k), n = (i16)((i16)(pct * v) / 10);
            if (n < v) { F(b, a, k) += F(b, b, k); F(b, b, k) = 0; }
            else { F(b, a, k) += n; F(b, b, k) -= n; }
        }
    }
}

void consider_nuclear_strike(int a, int b) {      // 1000:5EC3
    if (W(0xa12e + b * 2) == 5) nuclear_strike(7, b);
    if (NUKE_STAGE(a) == 10 && a != 7) {
        int w = WAR(a, b);
        if ((w > 6 && STATUS(a) > 5) || (POWER(b) <= POWER(a) && NUKE_STAGE(b) < 8 && w > 2)) nuclear_strike(a, b);
    }
    if (NUKE_STAGE(b) == 10) {
        int w = WAR(a, b);
        if ((w > 6 && STATUS(b) > 5) || (a == 7 && NUKE_STAGE(7) < 10) || (POWER(a) <= POWER(b) && NUKE_STAGE(a) < 8 && w > 2))
            nuclear_strike(b, a);
    }
}

// 1000:5FAD - decide whether the war between a and b is over; w/l preset a winner and loser (8 = none yet).
void resolve_war(int w, int l, int a, int b, int check) {
    if ((F(a, b, 0) < 1 || STATUS(a) == 10) && check) { w = b; l = a; }
    if ((F(b, a, 0) < 1 || STATUS(b) == 10) && check) { w = a; l = b; }
    if (b == 7 && W(0x98a0 + a * 2) > 4 && check) { w = 7; l = a; }
    if (b == 7 && W(0x98a0 + a * 2) < -4 && check) { w = a; l = 7; }
    if (w == 8) return;
    if (l == 7) {                                 // Israel is overrun: the game ends
        w = a; l = 7;
        war_ends(a, 7);
        game_over(2, 0, 4, a, 7);
    }
    STATUS(w) = 0;
    if (POWER(w) < 10) POWER(w)++;
    GNP(l) = (i16)w;
    war_ends(w, l);
    government_falls(l);
    for (int k = 1; k < 9; k++) {
        if (k != 3) F(w, w, k) += F(l, w, k) + F(w, l, k);
        F(l, w, k) = 0; F(w, l, k) = 0;
    }
    retarget(w);
}
