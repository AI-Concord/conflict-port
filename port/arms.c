// Arms purchases: suppliers, offers, quantities and deliveries (original 1000:9376 - 1000:A08E).
#include "conflict.h"

#define TURN            W(0xb268)
#define DEALS_TOTAL     W(0x929c)
#define DEALS_WITH(s)   W(0x9294 + (s) * 2)
#define PRICE(i)        W(0x411a + (i) * 2)
#define ON_ORDER(i, t)  W(0x98d2 + (i) * 100 + (t) * 2)     // units of weapon i ordered in month t

static int choose_quantity(int item) {            // 1000:9D3E
    char num[16]; char *spec = S(0xa3b4);
    int price = PRICE(item), lot = 1;
    int most = MONEY(7) / price;
    if (price < 0x3c) lot = 5;
    if (price < 10) lot = 10;
    if (price < 5) lot = 0x32;
    if (price > 0x3c) lot = 1;
    do most--; while (most % lot != 0);
    if (most <= lot || lot == 1) most = 0;
    int half = (lot + most) / 2;
    do half--; while (half % lot != 0);
    if (half <= lot || lot == 1) half = 0;
    strcpy(spec, S(0x4750)); int_to_str(lot, num); strcat(spec, num); strcat(spec, S(0x4763));
    if (half != 0) { strcat(spec, S(0x476a)); int_to_str(half, num); strcat(spec, num); strcat(spec, S(0x476c)); }
    if (most != 0) { strcat(spec, S(0x4773)); int_to_str(most, num); strcat(spec, num); strcat(spec, S(0x4775)); }
    strcat(spec, S(0x477c));
    int m = menu(0x1c2, 7, 0x276, 100, spec, 1);
    gfx_copy_from_page(0x1c2, 0x276, 0xc, 0xaf);
    gfx_copy_from_page(0xf0, 0x19a, 8, 0x161);
    recolor_fallen();
    if (half != 0 && most != 0) { if (m == 1) return lot; if (m == 2) return half; if (m == 3) return most; if (m == 4) return 0; }
    if (half == 0 && most != 0) { if (m == 1) return lot; if (m == 2) return most; if (m == 3) return 0; }
    if (half == 0 && most == 0) { if (m == 1) return lot; if (m == 2) return 0; }
    return 0;
}

static void buy_weapon(const i16 *list, int choice, int supplier) {     // 1000:999E
    char money[16], price[16], foot[80];
    int item = list[choice - 1];
    gfx_moveto(0xf0, 7);
    gfx_draw_picture(W(0x4166 + item * 2));
    int_to_str(MONEY(7), money);
    int_to_str(PRICE(item), price);
    strcpy(foot, S(0x4727)); strcat(foot, price); strcat(foot, S(0x4735)); strcat(foot, S(0x4739)); strcat(foot, money); strcat(foot, S(0x474c));
    strcpy(S(0xa1f2), S(UW(0x3cbc + item * 2)));
    strcpy(S(0xa2ba), S(UW(0x4082 + item * 2)));
    text_box(0xa5, 0x78, 0x1db, 0xc3, S(0xa1f2), S(0xa2ba), foot);
    gfx_moveto(0x15e, 0x10d);
    draw_unit_icon((const u8 *)S((u16)(W(0x4140 + item * 2) * 0x80 + UW(0xb29e))));
    int qty = choose_quantity(item);
    gfx_copy_from_page(0xa5, 0x1db, 0xd2, 0x155);
    if (qty != 0) {
        MONEY(7) -= (i16)(qty * PRICE(item));
        ON_ORDER(item, TURN) += (i16)qty;
        if (W(0x929e) != TURN) { DEALS_WITH(supplier)++; DEALS_TOTAL++; W(0x929e) = TURN; }
    }
}

static int list_offers(int supplier, i16 *list) { // 1000:9B48 - what this supplier will sell us now
    static const struct { int first, last, min_cash; } range[4] = {{0, 7, 99}, {7, 0xb, 0x31}, {0xb, 0xe, 0x31}, {0xe, 0x13, 99}};
    int n = 0, cash = MONEY(7);
    if (supplier < 0 || supplier > 3) return 0;
    if (supplier != 3 && !(W(0xb264) < 0x32)) return 0;       // governments refuse while Israel is condemned
    int loyalty = (DEALS_TOTAL == 0) ? 100 : (i16)(DEALS_WITH(supplier) * 100) / DEALS_TOTAL;
    for (int i = range[supplier].first; i < range[supplier].last; i++)
        if (W(0x40ce + i * 2) <= POWER(7) && W(0x40f4 + i * 2) <= loyalty && PRICE(i) < cash && cash > range[supplier].min_cash)
            list[n++] = (i16)i;
    return n;
}

static i16 arms_count_slot = 0;
i16 *arms_residue_count = &arms_count_slot;       // read before first assignment in the original

void arms_menu(void) {                            // 1000:9376
    char money[16], foot[80]; i16 list[20];
    char *title = S(0xa1f2), *body = S(0xa2ba), *spec = S(0xa3b4);
#define count (*arms_residue_count)
    if (TURN == 1) { DEALS_TOTAL = 0; for (int s = 0; s < 4; s++) DEALS_WITH(s) = 0; }
    int_to_str(MONEY(7), money);
    strcpy(title, S(0x4500)); strcpy(foot, S(0x451b));
    strcpy(title, S(0x452d)); strcpy(foot, S(0x4548)); strcat(foot, money); strcat(foot, S(0x455a));
    if (ATWAR(0) == 0 && ATWAR(3) == 0 && ATWAR(6) == 0) {
        strcpy(body, S(UW(0x4206 + (rnd(3) - 1) * 2)));
        int k = 3;
        if (F(7, 7, 7) < 0x32) k = 0;
        if (F(7, 7, 2) < 7) k = 1;
        if (F(7, 7, 3) < 200) k = 1;
        if (F(7, 7, 5) < 0x19) k = 2;
        strcat(body, S(UW(0x42c2 + k * 2)));
    } else { strcpy(body, S(0x455e)); strcat(body, S(0x457d)); }
    if (TURN < 2 && W(0x989c) == 0) strcpy(body, S(0x45b3));
    if (STATUS(2) == 10) recolor_fallen();
    text_box(300, 0x7d, 0x276, 0xbe, title, body, foot);
    int choice = menu(0x1c2, 5, 0x276, 0x6c, S(UW(0xca8)), 0);
    gfx_copy_from_page(300, 0x276, 0xda, 0x14c);
    gfx_copy_from_page(0x1c2, 0x276, 8, 0xbd);
    if (choice == 5) return;
    int supplier = choice - 1;
    choice = 0;
    for (;;) {
        if (count + 1 == choice) return;
        int_to_str(MONEY(7), money);
        strcpy(foot, S(0x45f2)); strcpy(foot, S(0x4604)); strcat(foot, money); strcat(foot, S(0x4616));
        count = (i16)list_offers(supplier, list);
        strcpy(title, S(0x461a)); strcat(title, S(UW(0x3b22 + supplier * 2))); strcat(title, S(0x4628));
        if (count == 0) {
            if ((supplier == 0 || supplier == 3) && MONEY(7) < 100) { strcpy(body, S(0x4635)); strcat(body, S(0x4658)); }
            if ((supplier == 1 || supplier == 2) && MONEY(7) < 0x32) { strcpy(body, S(0x4677)); strcat(body, S(0x469c)); }
            if (W(0xb264) > 0x31 && supplier != 3) { strcpy(body, S(0x46ba)); strcat(body, S(0x46e5)); }
            text_box(0x140, 0x82, 0x276, 0xc3, title, body, foot);
            menu(0x1c2, 5, 0x276, 0x32, S(UW(0xcc4)), 0);
            return;
        }
        strcpy(spec, S(0x470e));
        for (int i = 0; i < count; i++) { strcat(spec, S(UW(0x3cbc + list[i] * 2))); strcat(spec, S(0x471a)); }
        strcat(spec, S(0x471c));
        if (supplier == 3) strcpy(body, S(UW(0x44f0)));
        else strcpy(body, S(UW(0x44ea + (rnd(3) - 1) * 2)));
        int k = 6;
        if (POWER(7) > 3 && MONEY(7) > 0xfa) k = 5;
        if (POWER(7) > 3 && MONEY(7) > 500) k = 4;
        if (MONEY(7) < 200) k = 7;
        if (MONEY(7) > 1000) k = 10;
        if (DEALS_TOTAL > 5 && DEALS_TOTAL == DEALS_WITH(supplier)) k = 9;
        if (count < 3) k = 8;
        strcat(body, S(UW(0x44ea + k * 2)));
        text_box(0x118, 0x86, 0x276, 0xc3, title, body, foot);
        if (count < 5) choice = menu(0x186, 5, 0x276, 0x78, spec, 0);
        else if (count < 7) choice = menu(0x16d, 5, 0x276, 0x84, spec, 0);
        else choice = menu(0x16d, 5, 0x276, 0x96, spec, 0);
        gfx_copy_from_page(0x118, 0x276, 0xde, 0x155);
        gfx_copy_from_page(0x16d, 0x276, 8, 0xea);
        if (STATUS(2) == 10 || STATUS(3) == 10 || STATUS(6) == 10) recolor_fallen();
        if (count + 1 <= choice) continue;
        buy_weapon(list, choice, supplier);
        if ((supplier == 0 || supplier == 3) && MONEY(7) < 100) return;
        if ((supplier == 1 || supplier == 2) && MONEY(7) < 0x32) return;
        choice = menu(0x1a4, 5, 0x276, 0x4a, S(UW(0xcaa)), 0);
        gfx_copy_from_page(0x1a4, 0x276, 8, 0x81);
        if (STATUS(2) == 10 || STATUS(3) == 10 || STATUS(6) == 10) recolor_fallen();
        if (choice == 2) return;
    }
#undef count
}

void receive_deliveries(void) {                   // 1000:9FD9 - ordered weapons arrive one or two months later
    for (int i = 0; i < 0x13; i++) {
        int kind = W(0x40a8 + i * 2);
        if ((TURN > 1 && i < 0xe && i > 6) || (i < 7 && W(0xb264) < 0x1e)) {
            F(7, 7, kind) += ON_ORDER(i, TURN - 1); ON_ORDER(i, TURN - 1) = 0;
        }
        if (TURN > 2) { F(7, 7, kind) += ON_ORDER(i, TURN - 2); ON_ORDER(i, TURN - 2) = 0; }
    }
}
