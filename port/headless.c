// Headless driver for the stand-alone port: plays a key script and writes frames and
// memory dumps in the same format as the reference emulator, for comparison.
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <strings.h>
#include "conflict.h"
#include "ega.h"

int platform_load_data(const char *path);
static const char *gamedir = ".", *outdir = ".";
static FILE *script;
static u16 keyq[64]; static int kq_h, kq_t;
static int idle_polls, idle_limit = 3000, shot_n;
static unsigned long tick;

static void save_frame(const char *name) {
    char path[1024]; snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb"); if (!f) return;
    if (ega_mode == 0x10) {
        fprintf(f, "P6\n640 350\n255\n");
        for (int y = 0; y < 350; y++) for (int x = 0; x < 640; x++) {
            u32 off = (u32)(y * 80 + (x >> 3)); int bit = 7 - (x & 7), c = 0;
            for (int p = 0; p < 4; p++) c |= ((ega_plane[p][off] >> bit) & 1) << p;
            u8 e = ega_palette[c];
            fputc(((e >> 2) & 1) * 0xaa + ((e >> 5) & 1) * 0x55, f);
            fputc(((e >> 1) & 1) * 0xaa + ((e >> 4) & 1) * 0x55, f);
            fputc(((e >> 0) & 1) * 0xaa + ((e >> 3) & 1) * 0x55, f);
        }
    } else { fprintf(f, "P6\n1 1\n255\n"); fputc(0, f); fputc(0, f); fputc(0, f); }
    fclose(f);
}
static void dump_mem(const char *name) {          // same layout as the emulator: data segment at its DOS address
    char path[1024]; snprintf(path, sizeof path, "%s/%s.mem", outdir, name);
    FILE *f = fopen(path, "wb"); if (!f) return;
    static u8 zero[0xa0000]; memcpy(zero + (0x0120 + 0x128e) * 16, DSP, 0x10000);
    fwrite(zero, 1, sizeof zero, f); fclose(f);
}
void host_exit(void) { save_frame("final"); fprintf(stderr, "[port] exit\n"); exit(0); }
static u16 named_key(const char *s) {
    if (!strcmp(s, "enter")) return 0x1c0d; if (!strcmp(s, "esc")) return 0x011b; if (!strcmp(s, "space")) return 0x3920;
    if (!strcmp(s, "up")) return 0x4800; if (!strcmp(s, "down")) return 0x5000; if (!strcmp(s, "left")) return 0x4b00; if (!strcmp(s, "right")) return 0x4d00;
    if (!strcmp(s, "tab")) return 0x0f09; if (!strcmp(s, "bksp")) return 0x0e08;
    if (!strcmp(s, "home")) return 0x4700; if (!strcmp(s, "end")) return 0x4f00; if (!strcmp(s, "pgup")) return 0x4900; if (!strcmp(s, "pgdn")) return 0x5100;
    if (s[0] == 'f' && isdigit((unsigned char)s[1])) return (u16)((0x3a + atoi(s + 1)) << 8);
    if (s[0] == '0' && s[1] == 'x') return (u16)strtol(s, 0, 16);
    if (strlen(s) == 1) return (u16)(u8)s[0];
    return 0;
}
static void key_push(u16 k) { keyq[kq_t] = k; kq_t = (kq_t + 1) & 63; }
static int script_advance(void) {
    char line[256];
    while (script && fgets(line, sizeof line, script)) {
        char cmd[64] = "", arg[128] = "";
        if (sscanf(line, "%63s %127s", cmd, arg) < 1 || cmd[0] == '#') continue;
        if (!strcmp(cmd, "shot")) { char n[160]; if (arg[0]) snprintf(n, sizeof n, "%s", arg); else snprintf(n, sizeof n, "shot%03d", shot_n); shot_n++; save_frame(n); }
        else if (!strcmp(cmd, "dump")) dump_mem(arg[0] ? arg : "dump");
        else if (!strcmp(cmd, "key")) { key_push(named_key(arg)); return 1; }
        else if (!strcmp(cmd, "poke")) { unsigned a, v; if (sscanf(line, "%*s %x %x", &a, &v) == 2) W(a) = (i16)v; }
        else if (!strcmp(cmd, "quit")) host_exit();
    }
    return 0;
}
static void need_key(int blocking) {
    if (kq_h != kq_t) { idle_polls = 0; return; }
    if (!blocking && ++idle_polls < idle_limit) return;
    idle_polls = 0;
    if (!script_advance()) { fprintf(stderr, "[port] script ended\n"); host_exit(); }
}
int host_key_available(void) { need_key(0); return kq_h != kq_t; }
u16 host_key_read(void) { need_key(1); u16 k = keyq[kq_h]; kq_h = (kq_h + 1) & 63; return k; }
int host_clock_cs(void) { tick += 7; return (int)(i16)(u16)tick; }       // same deterministic clock as the emulator
void host_delay_ms(int ms) { (void)ms; }
FILE *host_fopen(const char *name, const char *mode) {
    char path[1024]; DIR *d = opendir(gamedir); struct dirent *e;
    if (d) { while ((e = readdir(d))) if (!strcasecmp(e->d_name, name)) { snprintf(path, sizeof path, "%s/%s", gamedir, e->d_name); closedir(d); return fopen(path, mode); } closedir(d); }
    if (mode[0] == 'w') { snprintf(path, sizeof path, "%s/%s", gamedir, name); return fopen(path, mode); }
    return NULL;
}
int main(int argc, char **argv) {
    const char *scr = NULL, *data = "conflict.dat";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d")) gamedir = argv[++i]; else if (!strcmp(argv[i], "-o")) outdir = argv[++i];
        else if (!strcmp(argv[i], "-s")) scr = argv[++i]; else if (!strcmp(argv[i], "-D")) data = argv[++i];
        else if (!strcmp(argv[i], "-i")) idle_limit = atoi(argv[++i]);
    }
    if (platform_load_data(data) != 0) { fprintf(stderr, "cannot load %s\n", data); return 1; }
    if (scr) script = fopen(scr, "r");
    ega_set_mode(3);
    game_main();
    return 0;
}
