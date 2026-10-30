// Hybrid-build registration: which original routines are replaced by ported code.
#include "hybrid.h"
#define emu_bridge_unused 0
#include <stdlib.h>
#include "ega.h"

u8 *DSP;
void need_key(int blocking); int key_avail(void); u16 key_pop(void);
FILE *open_ci(const char *name, const char *mode);

int host_key_available(void) { need_key(0); return key_avail(); }
u16 host_key_read(void) { need_key(1); return key_pop(); }
FILE *host_fopen(const char *name, const char *mode) { return open_ci(name, mode); }
int host_clock_cs(void) { return (i16)emu_call_far(0x20cd, 0x000c, 0); }
void host_set_video_mode(int mode) { ega_set_mode(mode); }
void host_delay_ms(int ms) { (void)ms; }
u16 game_malloc(u16 size) { return emu_call_far(0x21b8, 0x003d, 1, size); }
#include <setjmp.h>
extern jmp_buf game_restart;
static int native_main_running;
void game_longjmp(int value) { if (native_main_running) longjmp(game_restart, value); emu_call_far(0x20ba, 0x002c, 2, 0xb27e, value); }
void finish(int code);
void host_exit(void) { finish(0); }

static void h_gfx_init(void) { if (getenv("EMUDBG")) fprintf(stderr, "[dbg] DS=%04x SS=%04x SP=%04x\n", SG[3], SG[2], R[4]); RET(gfx_init()); }
static void h_gfx_load_pictures(void) { RET(gfx_load_pictures()); }
static void h_gfx_load_maps(void) { RET(gfx_load_maps()); }
static void h_gfx_moveto(void) { RET(gfx_moveto(A(0), A(1))); }
static void h_gfx_plot(void) { RET(gfx_plot(A(0), A(1))); }
static void h_gfx_hline(void) { gfx_hline(A(0), A(1), A(2), A(3)); }
static void h_gfx_vline(void) { gfx_vline(A(0), A(1), A(2), A(3)); }
static void h_gfx_fillrect(void) { gfx_fillrect(A(0), A(1), A(2), A(3)); }
static void h_gfx_text(void) { gfx_text(AS(0)); }
static void h_gfx_text_big(void) { gfx_text_big(AS(0)); }
static void h_gfx_text_width(void) { RET(gfx_text_width(AS(0))); }
static void h_gfx_draw_icon(void) { gfx_draw_icon((const u8 *)AS(0)); }
static void h_gfx_draw_picture(void) { gfx_draw_picture(A(0)); }
static void h_gfx_draw_map(void) { gfx_draw_map(A(0)); }
static void h_gfx_recolor(void) { gfx_recolor(A(0), A(1), A(2), A(3), A(4), A(5)); }
static void h_gfx_copy_from_page(void) { gfx_copy_from_page(A(0), A(1), A(2), A(3)); }
static void h_gfx_getkey(void) { RET(gfx_getkey()); }

static void h_rnd(void) { RET(rnd(A(0))); }
static void h_draw_box(void) { draw_box(A(0), A(1), A(2), A(3)); }
static void h_draw_titled_box(void) { draw_titled_box(A(0), A(1), A(2), A(3), AS(4)); }
static void h_contains4(void) { RET(contains4(AS(0), AS(1))); }
static void h_str_overlay(void) { str_overlay(AS(0), AS(1), A(2)); }
static void h_squeeze_spaces(void) { squeeze_spaces(AS(0)); }
static void h_wait_key(void) { RET(wait_key()); }
static void h_text_mode(void) { text_mode(); }
static void h_show_status_chart(void) { show_status_chart(); }
static void h_calibrate_delay(void) { calibrate_delay(); }
static void h_load_fonts(void) { load_fonts(); }
static void h_text_box(void) { text_box(A(0), A(1), A(2), A(3), AS(4), AS(5), AS(6)); }
static void h_menu(void) { RET(menu(A(0), A(1), A(2), A(3), AS(4), A(5))); }
static void h_menu_item(void) { menu_item(A(0), A(1), A(2), AS(3)); }
static void h_menu_select(void) { RET(menu_select(A(0), A(1))); }

static void h_draw_newspapers(void) { draw_newspapers(); }
static void h_draw_unit_icon(void) { draw_unit_icon((const u8 *)AS(0)); }
static void h_intro_screen(void) { intro_screen(); }
static void h_title_screen(void) { title_screen(); }
static void h_thinking_start(void) { thinking_start(); }
static void h_thinking_dot(void) { thinking_dot(); }
static void h_icon_wallpaper(void) { icon_wallpaper(A(0)); }
static void h_save_game(void) { save_game(); }
static void h_load_game(void) { load_game(); }
static void h_new_game_init(void) { new_game_init(); }
static void h_index_news_by_country(void) { index_news_by_country(); }

extern i16 *ct_residue_rel, *ct_residue_r;
static i16 *stack_slot(int off) { return (i16 *)(mem + ((u32)SG[2] << 4) + (u16)(R[4] + off)); }
static void h_country_turn(void) {
    // keep the emulated stack pointer below the original frame so callees leave these slots alone
    ct_residue_rel = stack_slot(-0x30); ct_residue_r = stack_slot(-0x2c);
    int p = A(0); R[4] -= 0x40; country_turn(p); R[4] += 0x40;
}
static void h_sour_relations(void) { sour_relations(A(0), A(1), A(2)); }
static void h_seek_alliance(void) { seek_alliance(A(0), A(1)); }

static void h_flash_war_icon(void) { flash_war_icon(A(0), A(1)); }
static void h_flash_nuke_icon(void) { flash_nuke_icon(A(0), A(1), A(2)); }
static void h_map_nuke_site(void) { map_nuke_site(A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7), A(8)); }
static void h_map_fallen_marker(void) { map_fallen_marker(A(0), A(1), A(2)); }
static void h_map_war_marker(void) { map_war_marker(A(0), A(1), A(2), A(3), A(4)); }
static void h_map_overlay_libya(void) { map_overlay_libya(); }
static void h_map_overlay_egypt(void) { map_overlay_egypt(); }
static void h_map_overlay_syria(void) { map_overlay_syria(); }
static void h_map_overlay_israel(void) { map_overlay_israel(); }
static void h_month_name(void) { RET(month_name(A(0))); }

extern i16 *briefing_residue_choice;
static void h_int_to_str(void) { int_to_str(A(0), AS(1)); }
static void h_expand_template(void) { expand_template(stk(0), A(1), A(2), AS(3)); }
static void h_add_headline(void) { add_headline(stk(0), A(1), A(2)); }
static void h_add_story(void) { add_story(stk(0), A(1), A(2)); }
static void h_retarget(void) { retarget(A(0)); }
static void h_absorb_forces(void) { absorb_forces(A(0)); }
static void h_government_falls(void) { government_falls(A(0)); }
static void h_redeploy_border_troops(void) { redeploy_border_troops(); }
static void h_world_events(void) { world_events(); }
static void h_recolor_fallen(void) { recolor_fallen(); }
static void h_foreign_office_comment(void) { foreign_office_comment(); }
static void h_country_screen(void) { country_screen(A(0)); }
static void h_monthly_briefing(void) { briefing_residue_choice = stack_slot(-0xb8); R[4] -= 0xc0; monthly_briefing(); R[4] += 0xc0; }

static void h_commit_forces(void) { commit_forces(A(0), A(1)); }
static void h_consider_nuclear_strike(void) { consider_nuclear_strike(A(0), A(1)); }
static void h_resolve_war(void) { resolve_war(A(0), A(1), A(2), A(3), A(4)); }

static void h_diplomacy_screen(void) { diplomacy_screen(A(0)); }
static void h_intel_screen(void) { intel_screen(A(0)); }
static void h_covert_operations(void) { covert_operations(); }
static void h_weapon_name(void) { RET(weapon_name(A(0))); }
static void h_weapon_blurb(void) { RET(weapon_blurb(A(0))); }

static void h_front_report(void) { RET(front_report(A(0))); }
static void h_strategic_menu(void) { strategic_menu(); }

static void h_border_command(void) { border_command(A(0)); }

extern i16 *arms_residue_count;
static void h_arms_menu(void) { arms_residue_count = stack_slot(-0x1c); R[4] -= 0x48; arms_menu(); R[4] += 0x48; }
static void h_receive_deliveries(void) { receive_deliveries(); }

static void h_forces_review(void) { forces_review(); }
static void h_policing_menu(void) { policing_menu(); }
static void h_nuclear_policy(void) { nuclear_policy(); }
static void h_game_over(void) { game_over(A(0), A(1), A(2), A(3), A(4)); }
static void h_nuclear_strike(void) { nuclear_strike(A(0), A(1)); }
static void h_war_ends(void) { war_ends(A(0), A(1)); }
static void h_game_main(void) {
    // The emulated clock is read once for the seed, as the original did.
    native_main_running = 1; R[4] -= 0x200; game_main();
}

static void h_israeli_strikes(void) { israeli_strikes(); }
static void h_hostile_acts(void) { hostile_acts(); }

static void h_make_peace(void) { make_peace(A(0), A(1)); }
static void h_war_month(void) { war_month(); }

static void h_relations_changed(void) { relations_changed(A(0), A(1)); }
static void h_unrest_news(void) { unrest_news(A(0)); }
static void h_relations_news(void) { relations_news(A(0), A(1)); }
static void h_battle_news(void) { battle_news(A(0), A(1), A(2), A(3)); }
static void h_strike_news(void) { strike_news(A(0), A(1), A(2), A(3)); }
static void h_peace_news(void) { peace_news(A(0), A(1), A(2)); }
static void h_world_news(void) { world_news(A(0)); }
static void h_summit(void) { summit(); }
static void h_yearly_review(void) { yearly_review(A(0)); }

void port_register_hooks(void) {
    DSP = mem + ((u32)(load_seg + 0x128e) << 4);
    HOOK(1f39, 0771, gfx_init);
    HOOK(1f39, 0783, gfx_load_pictures);
    HOOK(1f39, 078b, gfx_load_maps);
    HOOK(1f39, 0801, gfx_moveto);
    HOOK(1f39, 0813, gfx_plot);
    HOOK(1f39, 0827, gfx_hline);
    HOOK(1f39, 0858, gfx_vline);
    HOOK(1f39, 091e, gfx_fillrect);
    HOOK(1f39, 08e4, gfx_text);
    HOOK(1f39, 08f8, gfx_text_big);
    HOOK(1f39, 09e9, gfx_text_width);
    HOOK(1f39, 07df, gfx_draw_icon);
    HOOK(1f39, 0746, gfx_draw_picture);
    HOOK(1f39, 0937, gfx_draw_map);
    HOOK(1f39, 0889, gfx_recolor);
    HOOK(1f39, 06dd, gfx_copy_from_page);
    HOOK(1f39, 0a58, gfx_getkey);
    HOOK(1000, 0010, rnd);
    HOOK(1000, 00b3, draw_box);
    HOOK(1000, 0286, draw_titled_box);
    HOOK(1000, 0357, contains4);
    HOOK(1000, 03bd, str_overlay);
    HOOK(1000, 03e5, squeeze_spaces);
    HOOK(1000, 0444, wait_key);
    HOOK(1000, 0479, text_mode);
    HOOK(1000, 0491, show_status_chart);
    HOOK(1000, 0739, calibrate_delay);
    HOOK(1000, 07fc, load_fonts);
    HOOK(1000, 0a7c, text_box);
    HOOK(1000, 0d24, menu);
    HOOK(1000, 0e13, menu_item);
    HOOK(1000, 1044, menu_select);
    HOOK(1000, 14bc, draw_newspapers);
    HOOK(1000, b8ab, draw_unit_icon);
    HOOK(1000, 2058, intro_screen);
    HOOK(1000, 2337, title_screen);
    HOOK(1000, 245c, thinking_start);
    HOOK(1000, 24a1, thinking_dot);
    HOOK(1000, 24c2, icon_wallpaper);
    HOOK(1000, 25b3, save_game);
    HOOK(1000, 29ed, load_game);
    HOOK(1000, 2e39, new_game_init);
    HOOK(1000, 3036, index_news_by_country);
    HOOK(1000, 3138, country_turn);
    HOOK(1000, 3897, sour_relations);
    HOOK(1000, 3a23, seek_alliance);
    HOOK(1000, 4384, flash_war_icon);
    HOOK(1000, 44d9, flash_nuke_icon);
    HOOK(1000, 4126, map_nuke_site);
    HOOK(1000, 42a5, map_fallen_marker);
    HOOK(1000, 42f5, map_war_marker);
    HOOK(1000, 3d5e, map_overlay_libya);
    HOOK(1000, 3e40, map_overlay_egypt);
    HOOK(1000, 3f03, map_overlay_syria);
    HOOK(1000, 3fc9, map_overlay_israel);
    HOOK(1000, 4652, month_name);
    HOOK(1d93, 1996, int_to_str);
    HOOK(1000, 6d35, expand_template);
    HOOK(1000, 6c39, add_headline);
    HOOK(1000, 6cb7, add_story);
    HOOK(1000, 6e8e, retarget);
    HOOK(1000, 595d, absorb_forces);
    HOOK(1000, 58be, government_falls);
    HOOK(1000, 5a52, redeploy_border_troops);
    HOOK(1000, 56c7, world_events);
    HOOK(1000, 4f69, recolor_fallen);
    HOOK(1000, 502c, foreign_office_comment);
    HOOK(1000, 549b, country_screen);
    HOOK(1000, 4675, monthly_briefing);
    HOOK(1000, 5aa5, commit_forces);
    HOOK(1000, 5ec3, consider_nuclear_strike);
    HOOK(1000, 5fad, resolve_war);
    HOOK(1000, 61b0, diplomacy_screen);
    HOOK(1000, 6fa6, intel_screen);
    HOOK(1000, 74e1, covert_operations);
    HOOK(1000, 76d4, weapon_name);
    HOOK(1000, 76e4, weapon_blurb);
    HOOK(1000, 76f4, front_report);
    HOOK(1000, 7e9c, strategic_menu);
    HOOK(1000, 8462, border_command);
    HOOK(1000, 9376, arms_menu);
    HOOK(1000, 9fd9, receive_deliveries);
    HOOK(1000, a08f, forces_review);
    HOOK(1000, a86d, policing_menu);
    HOOK(1000, aac8, nuclear_policy);
    HOOK(1000, b5b5, game_over);
    HOOK(1000, ad9b, nuclear_strike);
    HOOK(1000, ae6c, war_ends);
    HOOK(1b92, 0714, israeli_strikes);
    HOOK(1b92, 0007, hostile_acts);
    HOOK(1b92, 141b, make_peace);
    HOOK(1b92, 0bd1, war_month);
    HOOK(1d93, 0007, relations_changed);
    HOOK(1d93, 037b, unrest_news);
    HOOK(1d93, 0468, relations_news);
    HOOK(1d93, 0599, battle_news);
    HOOK(1d93, 0931, strike_news);
    HOOK(1d93, 0ad0, peace_news);
    HOOK(1d93, 0b2a, world_news);
    HOOK(1d93, 0c02, summit);
    HOOK(1d93, 111f, yearly_review);
    if (!getenv("NO_NATIVE_MAIN")) HOOK(1000, b681, game_main);
}
