// Conflict: Middle East Political Simulator - native port.
// Game state lives in an image of the original program's data segment (DGROUP);
// globals are addressed by their original offsets so data tables, strings and
// saved games stay compatible with the DOS release.
#ifndef CONFLICT_H
#define CONFLICT_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int16_t i16; typedef uint16_t u16; typedef uint8_t u8; typedef int32_t i32; typedef uint32_t u32;

extern u8 *DSP;                                   // base of the data segment image
#define W(o)  (*(i16 *)(DSP + (u16)(o)))          // signed word
#define UW(o) (*(u16 *)(DSP + (u16)(o)))          // unsigned word
#define B(o)  (*(u8 *)(DSP + (u16)(o)))           // byte
static inline char *S(u16 o) { return (char *)DSP + o; }           // near pointer -> host pointer
static inline u16 OFF(const void *p) { return (u16)((const u8 *)p - DSP); }

// ---- host services (platform layer) ----
int host_key_available(void);                     // non-blocking
u16 host_key_read(void);                          // blocking; BIOS code: scancode<<8 | ascii
FILE *host_fopen(const char *name, const char *mode);
int host_clock_cs(void);                          // time of day in 1/100 s
void host_set_video_mode(int mode);
u16 game_malloc(u16 size);                        // near-heap allocation inside the data segment
void game_longjmp(int value);                     // unwinds to the main loop (original used setjmp/longjmp)
void host_delay_ms(int ms);                       // replaces the original busy-wait loops

// The C runtime's rand(): state kept in the data segment so sequences match the original.
int game_rand(void);
void game_srand(u16 seed);
int zt_fgetc(FILE *f);                            // text-mode fgetc as the Zortech library implemented it

// ---- graphics library (was the assembly module at segment 1F39) ----
#define G_COLOR      B(0xb2a2)                    // current drawing colour
#define G_BGCOLOR    B(0xb2a0)                    // background / fill colour
#define G_FONT       UW(0xb290)                   // near pointer to the 8-pixel font
#define G_BIGFONT    UW(0xb292)                   // near pointer to the 16-pixel font
int  gfx_init(void);
int  gfx_load_pictures(void);
int  gfx_load_maps(void);
int  gfx_moveto(int x, int y);
int  gfx_plot(int x, int y);
void gfx_hline(int x1, int x2, int y, int use_color);
void gfx_vline(int y1, int y2, int x, int use_color);
void gfx_fillrect(int x1, int x2, int y1, int y2);
void gfx_text(const char *s);
void gfx_text_big(const char *s);
int  gfx_text_width(const char *s);
void gfx_draw_icon(const u8 *icon);
void gfx_draw_picture(int n);
void gfx_draw_map(int n);
void gfx_recolor(int x, int y, int w, int h, int from, int to);
void gfx_copy_from_page(int x1, int x2, int y1, int y2);
int  gfx_getkey(void);
extern int gfx_plain_keys;                        // 1: any key ends a pause (see gfx.c)

// ---- game state (names are provisional; offsets are the original globals) ----
#define REL(a, b)     W(0xa05e + (a) * 0x10 + (b) * 2)            // relations, indexed with a > b
#define F(c, t, k)    W(0x93a0 + (c) * 0xa0 + (t) * 0x14 + (k) * 2)  // forces of c facing t (t == c: at home), kind k
#define MONEY(c)      W(0x9390 + (c) * 2)
#define STATUS(c)     W(0xa10e + (c) * 2)       // 10 = government has fallen
#define STABILITY(c)  W(0xa0ce + (c) * 2)
#define TARGET(c)     W(0xa14e + (c) * 2)       // country c is currently focused on
#define POWER(c)      W(0xa04e + (c) * 2)
#define UNREST(c)     W(0xa11e + (c) * 2)
#define GNP(c)        W(0x92f0 + (c) * 2)
#define BUDGET_PCT(c) W(0xa03e + (c) * 2)
#define NUKE_STAGE(c) W(0xa0fe + (c) * 2)
#define ATWAR(c)      W(0x9380 + (c) * 2)
static inline i16 rel_get(int a, int b) { return a > b ? REL(a, b) : REL(b, a); }
static inline void rel_set(int a, int b, int v) { if (a > b) REL(a, b) = (i16)v; else REL(b, a) = (i16)v; }

// ---- ui.c ----
int  rnd(int n);
void draw_box(int x1, int y1, int x2, int y2);
void draw_titled_box(int x1, int y1, int x2, int y2, const char *title);
int  contains4(const char *s, const char *pat);
void str_overlay(char *dst, const char *src, int pos);
void squeeze_spaces(char *s);
int  wait_key(void);
void text_mode(void);
void show_status_chart(void);
void calibrate_delay(void);
void load_fonts(void);
void text_box(int x1, int y1, int x2, int y2, const char *title, const char *text, const char *extra);
int  menu(int x1, int y1, int x2, int y2, const char *spec, int country);
void menu_item(int x, int y, int w, const char *text);
int  menu_select(int x2, int y);

// ---- paper.c ----
void draw_newspapers(void);
void draw_unit_icon(const u8 *icon);
void intro_screen(void);
void title_screen(void);
void thinking_start(void);
void thinking_dot(void);
void icon_wallpaper(int kind);
void save_game(void);
void load_game(void);
void new_game_init(void);
void index_news_by_country(void);

// ---- ai.c ----
void country_turn(int p);
void ai_month_begins(int new_game);
void sour_relations(int p, int t, int rel);
void seek_alliance(int p, int t);

// ---- maps.c ----
void delay_loop(double base);
int  delay_or_key(double base);
void flash_war_icon(int x, int y);
void flash_nuke_icon(int x, int y, int c);
void map_nuke_site(int c, int prev_stage, int seen_turn, int x1, int y1, int x2, int y2, int ix, int iy);
void map_fallen_marker(int c, int x, int y);
void map_war_marker(int a, int b, int seen_turn, int x, int y);
void map_overlay_libya(void);
void map_overlay_egypt(void);
void map_overlay_syria(void);
void map_overlay_israel(void);
u16  month_name(int m);

// ---- briefing.c ----
void int_to_str(int v, char *out);
void expand_template(u16 tmpl, int a, int b, char *out);
void add_headline(u16 tmpl, int a, int b);
void add_story(u16 tmpl, int a, int b);
void retarget(int p);
void absorb_forces(int c);
void government_falls(int c);
void redeploy_border_troops(void);
void world_events(void);
void recolor_fallen(void);
void foreign_office_comment(void);
void country_screen(int c);
void monthly_briefing(void);

// ---- war.c ----
void commit_forces(int a, int b);
void consider_nuclear_strike(int a, int b);
void resolve_war(int w, int l, int a, int b, int check);

// ---- diplomacy.c ----
void diplomacy_screen(int p);
void intel_screen(int p);
void covert_operations(void);
u16  weapon_name(int n);
u16  weapon_blurb(int n);

// ---- fronts.c ----
u16  front_report(int c);
void strategic_menu(void);

// ---- border.c ----
void border_command(int which);

// ---- arms.c ----
void arms_menu(void);
void receive_deliveries(void);

// ---- policy.c ----
void forces_review(void);
void policing_menu(void);
void nuclear_policy(void);
void game_over(int reason, int paper, int wallpaper, int a, int b);
void nuclear_strike(int a, int b);
void war_ends(int winner, int loser);
void game_main(void);
void host_exit(void);

// ---- strikes.c ----
void israeli_strikes(void);
void hostile_acts(void);

// ---- warsim.c ----
void make_peace(int a, int b);
void war_month(void);

// ---- news.c ----
void relations_changed(int a, int b);
void unrest_news(int c);
void relations_news(int a, int b);
void battle_news(int a, int b, int kind, int amount);
void strike_news(int kind, int result, int a, int b);
void peace_news(int a, int b, int kind);
void world_news(int kind);
void summit(void);
void yearly_review(int year);
void init_countries(void);
#endif
