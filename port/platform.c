// Stand-alone platform layer: the data segment image, near heap and restart jump.
// Input, timing and file locations are supplied by the front end (headless.c or the Mac app).
#include <stdlib.h>
#include <setjmp.h>
#include "conflict.h"
#include "ega.h"

static u8 dgroup[0x10000];
u8 *DSP = dgroup;
static u16 heap_next;
extern jmp_buf game_restart;

int platform_load_data(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    memset(dgroup, 0, sizeof dgroup);
    size_t n = fread(dgroup, 1, sizeof dgroup, f);
    fclose(f);
    heap_next = 0xc100;                           // above the original's static data and stack
    return n > 0x8000 ? 0 : -1;
}

u16 game_malloc(u16 size) {
    u16 p = heap_next;
    size = (u16)((size + 3) & ~1u);
    if ((u32)p + size > 0xff00) return 0;
    heap_next = (u16)(p + size);
    return p;
}

void game_longjmp(int value) { longjmp(game_restart, value); }
void host_set_video_mode(int mode) { ega_set_mode(mode); }
