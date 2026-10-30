// Monthly decisions of the computer-controlled countries (original 1000:3138 - 1000:3D5D).
#include <stdlib.h>
#include "conflict.h"

void sour_relations(int p, int t, int rel) {      // 1000:3897 - p turns against its target t
    if (UNREST(t) < 10) { MONEY(p) -= 2; UNREST(t)++; }
    if (POWER(t) <= POWER(p) && F(p, t, 9) == 1 && rel > 1) rel--;
    if (POWER(p) < POWER(t) && F(p, t, 9) == 1 && rel > 2) rel--;
    if (POWER(t) <= POWER(p) && F(p, t, 9) == 2 && rel > 4) rel--;
    if (POWER(p) < POWER(t) && F(p, t, 9) == 0 && rel > 5) rel--;
    if (t < p && REL(p, t) != rel) { REL(p, t) = (i16)rel; relations_changed(p, t); }
    if (p < t && REL(t, p) != rel) { REL(t, p) = (i16)rel; relations_changed(p, t); }
}

void seek_alliance(int p, int t) {                // 1000:3A23 - p courts t with aid, pacts and arms
    int rel = (t < p) ? REL(p, t) : REL(t, p);
    if (rel < 10 && MONEY(p) > 100) {
        MONEY(p) -= 100; MONEY(t) += 100;
        if (rel == 9 && POWER(p) < 10) POWER(p)++;
        if (rel == 9) TARGET(t) = TARGET(p);
        if (t < p) REL(p, t)++;
        if (p < t) REL(t, p)++;
        relations_changed(p, t);
    }
    if (rel == 10) {
        if (F(t, TARGET(p), 9) == 1) {
            i16 aid = (i16)((double)MONEY(p) * 0.4);
            MONEY(p) -= aid; MONEY(t) += aid;
        }
        int e = TARGET(p);
        if (REL(e, t) == 1 || REL(t, e) == 1) {
            if (F(p, p, 0) > 500) { F(p, p, 0) -= 200; F(t, t, 0) += 200; }
            if (F(p, t, 9) != 0) {
                for (int k = 1; k < 9; k++) if (k != 2) {
                    i16 n = F(p, p, k) / 3;
                    F(p, p, k) -= n; F(t, t, k) += n;
                }
            }
            for (int i = 0; i < 8; i++) {           // the ally's relations drift towards p's own
                int rt = (i < t) ? REL(t, i) : REL(i, t);
                int rp = (i < p) ? REL(p, i) : REL(i, p);
                if ((rt < 4 && F(t, i, 9) == 1 && rt > 1) || rt > 3) {
                    int d = rp - rt;
                    rt += (d > 0) - (d < 0);
                }
                if (i < t) REL(t, i) = (i16)rt; else REL(i, t) = (i16)rt;
            }
        }
    }
}

// The original reads two locals before writing them, so their values are whatever the
// previous call left on the stack. These pointers stand in for those stack slots.
static i16 residue_rel_slot, residue_r_slot;
i16 *ct_residue_rel = &residue_rel_slot, *ct_residue_r = &residue_r_slot;
// What those slots held in the original at the first call of a month, established by
// tracing the reference emulator. The memory was last used either by world_events() at the
// end of the previous month - leaving zero unless its last step reported a change of
// relations - or, if covert operations produced news this month, by the 100-byte text
// buffer of that news item (bytes 88-89 and 92-93 of it; zero for any shorter text).
// A zero in the second slot lets the first country advance a late-stage nuclear
// programme, so this is reproduced rather than "fixed".
extern int world_events_ended_with_news, covert_news_posted;
extern char last_news_text[];
void ai_month_begins(int new_game) {
    residue_rel_slot = 0;
    residue_r_slot = (new_game || world_events_ended_with_news) ? 0x4000 : 0;
    if (!new_game && covert_news_posted) {
        const u8 *t = (const u8 *)last_news_text;
        residue_rel_slot = (i16)(t[88] | (t[89] << 8));
        residue_r_slot = (i16)(t[92] | (t[93] << 8));
    }
    if (getenv("RESIDUE_DEBUG")) fprintf(stderr, "[model] new=%d r=%d\n", new_game, residue_r_slot);
}
#define rel (*ct_residue_rel)

void country_turn(int p) {                        // 1000:3138
    int stance = 0;      // 1: has a deadly enemy still standing, 2: a hostile neighbour
    int need = 0;
    int t = TARGET(p);
    if (STATUS(t) > 9) { absorb_forces(t); retarget(p); }
    if (STATUS(p) > 9) { government_falls(p); return; }
    {
        int a = UNREST(p), b = STATUS(p), c = POWER(p);
        if (rnd(10) < a + b - c && (ATWAR(p) == 0 || STATUS(p) != 9)) { STATUS(p)++; unrest_news(p); }
    }
    if (STATUS(p) == 10) { government_falls(p); return; }

    MONEY(p) = (i16)((double)MONEY(p) + (double)GNP(p) * ((double)(float)(double)BUDGET_PCT(p) / 100.0));

    for (int i = 0; i < 8; i++) {
        if (p < i) rel = REL(i, p);
        if (i < p) rel = REL(p, i);
        if (rel == 1 && STATUS(i) < 10) stance = 1;
        if (rel == 2 && stance == 0 && STATUS(i) < 10) stance = 2;
    }
    if (ATWAR(p) == 0) {                            // move forces to or from the Israeli border
        for (int k = 0; k < 9; k++) {
            need = F(7, p, k) - F(p, 7, k);
            if (STABILITY(p) == 3 && stance != 1 && TARGET(p) == 7) need = (F(p, p, k) / 3) << 1;
            if ((STABILITY(p) > 3 && need < 0) || (need > 0 && need <= F(p, p, k))) { F(p, 7, k) += (i16)need; F(p, p, k) -= (i16)need; }
            if (need > 0 && F(p, p, k) < need) { F(p, 7, k) += F(p, p, k); F(p, p, k) = 0; }
        }
        if (TARGET(p) == 7 && STABILITY(p) < 3 && F(p, 7, 0) != 0 && ATWAR(p) == 0) { W(0x98a0 + p * 2) -= 2; STABILITY(p) = 1; }
    }
    for (int i = 0; i < 8; i++) {
        if (stance != 1) {
            rel = (i < p) ? REL(p, i) : REL(i, p);
            if (i == TARGET(p) && i != p && STATUS(i) < 10) {
                if ((double)F(p, p, 0) * 1.5 > (double)F(i, i, 0) || (double)F(p, i, 0) * 0x1.3333333333334p+0 > (double)F(i, p, 0))
                    sour_relations(p, i, rel);
            }
        }
        if (i != 7 && i != TARGET(p) && p != TARGET(i) && STATUS(i) < 8 && F(p, i, 9) != 0 && rel > 6 && p != i) {
            int a = POWER(p), b = POWER(i);
            if (b < a || (a == b && GNP(i) < GNP(p))) seek_alliance(p, i);
        }
    }
    switch ((u16)NUKE_STAGE(p)) {                   // nuclear programme
    case 0: case 10: break;
    case 1: case 2: case 3: case 4: case 5: case 6:
        if (MONEY(p) > 0x32 && POWER(p) >= 4) { MONEY(p) -= 0x32; NUKE_STAGE(p)++; }
        break;
    case 7: case 8: case 9:
        if (MONEY(p) > 0x14) { if (*ct_residue_r <= 3) NUKE_STAGE(p)++; MONEY(p) -= 0x14; }
        if (NUKE_STAGE(p) == 10) world_news(4);
        break;
    default: puts(S(0x1a03)); break;
    }
    if (stance == 2) {                              // spend what is left on arms
        while (MONEY(p) > 0x28) { F(p, p, 3)++; MONEY(p) -= 0x23; }
        while (MONEY(p) > 5) { F(p, p, 1)++; MONEY(p)--; }
    }
    if (stance == 1) while (MONEY(p) > 1) { F(p, p, 1)++; MONEY(p)--; }
#undef rel
    if (stance == 0) {
        int r = rnd(8);
        *ct_residue_r = (i16)r;
        while (F(p, p, 2) < 3) { F(p, p, 2)++; MONEY(p) -= 0x19; }
        while (r == 2 && MONEY(p) > 0x32) { F(p, p, 3)++; MONEY(p) -= 0x23; }
        while (r == 3 && MONEY(p) > 5) { F(p, p, 7)++; MONEY(p) -= 5; }
        while (r == 4 && MONEY(p) > 3) { F(p, p, 5)++; MONEY(p) -= 3; }
        if (r >= 5 && MONEY(p) > 0x78) { F(p, p, 1) += 100; MONEY(p) -= 100; }
    }
}
