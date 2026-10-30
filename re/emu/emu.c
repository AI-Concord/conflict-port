// Minimal 8086 + DOS + EGA emulator used as a reference oracle for the Conflict port.
// Headless: driven by a key script, dumps EGA frames as PPM and memory snapshots.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8; typedef int16_t s16; typedef int32_t s32;

u8 mem[0x110000];
u16 R[8];            // AX CX DX BX SP BP SI DI
u16 SG[4];            // ES CS SS DS
static u16 ip; static u16 flags = 0x0002;
enum {AX,CX,DX,BX,SP,BP,SI,DI}; enum {ES,CS,SS,DS};
#define CF 0x001
#define PF 0x004
#define AF 0x010
#define ZF 0x040
#define SF 0x080
#define TF 0x100
#define IF 0x200
#define DF 0x400
#define OF 0x800

void emu_hook_stats(void);
static unsigned long long icount = 0;
int trace = 0;
static const char *gamedir = ".";
static const char *savedir = NULL;
static const char *outdir = ".";
#ifdef EMU_LIB
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
static pthread_mutex_t kmx = PTHREAD_MUTEX_INITIALIZER; static pthread_cond_t kcv = PTHREAD_COND_INITIALIZER;
static volatile int engine_exited = 0; static volatile int engine_quit = 0;
#endif

// ---------------- EGA (shared with the port) ----------------
#include "../../port/ega.h"
#define plane ega_plane
#define latch ega_latch
#define gc ega_gc
#define seq ega_seq
#define video_mode ega_mode
#define ega_pal ega_palette
#define set_mode ega_set_mode
static u8 gc_idx, seq_idx;
static unsigned long long vwrites = 0;
void port_out(u16 port, u8 v) {
    switch (port) {
    case 0x3c4: seq_idx = v & 7; break;
    case 0x3c5: seq[seq_idx] = v; break;
    case 0x3ce: gc_idx = v & 15; break;
    case 0x3cf: gc[gc_idx] = v; break;
    default: if (trace) fprintf(stderr, "OUT %04x,%02x\n", port, v);
    }
}
static u8 port_in(u16 port) {
    static u8 t; if (port == 0x3da) { t ^= 9; return t; }
    if (port == 0x3cf) return gc[gc_idx]; if (port == 0x3c5) return seq[seq_idx];
    return 0xff;
}
static void save_frame(const char *name) {
    char path[1024]; snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb"); if (!f) { perror(path); return; }
    if (video_mode == 0x10 || video_mode == 0x0e || video_mode == 0x0d) {
        int w = video_mode == 0x0d ? 320 : 640, h = video_mode == 0x10 ? 350 : 200;
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
            u32 off = y * (w / 8) + (x >> 3); int bit = 7 - (x & 7); int c = 0;
            for (int p = 0; p < 4; p++) c |= ((plane[p][off] >> bit) & 1) << p;
            u8 e = ega_pal[c];
            u8 r = ((e >> 2) & 1) * 0xaa + ((e >> 5) & 1) * 0x55;
            u8 g = ((e >> 1) & 1) * 0xaa + ((e >> 4) & 1) * 0x55;
            u8 b = ((e >> 0) & 1) * 0xaa + ((e >> 3) & 1) * 0x55;
            fputc(r, f); fputc(g, f); fputc(b, f);
        }
    } else { // text mode dump as tiny image placeholder + text file
        fprintf(f, "P6\n1 1\n255\n"); fputc(0, f); fputc(0, f); fputc(0, f);
        char tp[1024]; snprintf(tp, sizeof tp, "%s/%s.txt", outdir, name); FILE *t = fopen(tp, "w");
        if (t) { for (int y = 0; y < 25; y++) { for (int x = 0; x < 80; x++) { u8 c = mem[0xb8000 + (y * 80 + x) * 2]; fputc(c >= 32 && c < 127 ? c : ' ', t); } fputc('\n', t); } fclose(t); }
    }
    fclose(f);
}

// ---------------- memory access ----------------
static inline u8 rb(u32 a) { a &= 0xfffff; if (a >= 0xa0000 && a < 0xb0000 && video_mode >= 0x0d) return ega_read(a - 0xa0000); return mem[a]; }
static u32 watch_addr = 0xffffffff; static u16 watch_cs, watch_ip; static u16 ip_now(void);
static inline void wb(u32 a, u8 v) { a &= 0xfffff; if (a == watch_addr) { watch_cs = SG[1]; watch_ip = ip_now(); } if (a >= 0xa0000 && a < 0xb0000 && video_mode >= 0x0d) { ega_write(a - 0xa0000, v); return; } mem[a] = v; }
static inline u16 rw(u32 a) { return rb(a) | (rb(a + 1) << 8); }
static inline void ww(u32 a, u16 v) { wb(a, v & 0xff); wb(a + 1, v >> 8); }
#define LIN(seg, off) ((((u32)(seg)) << 4) + (u16)(off))
static inline u8 fetch8(void) { u8 v = mem[LIN(SG[CS], ip)]; ip++; return v; }
static inline u16 fetch16(void) { u16 v = fetch8(); v |= fetch8() << 8; return v; }
static u16 ip_now(void) { return ip; }
void push(u16 v) { R[SP] -= 2; ww(LIN(SG[SS], R[SP]), v); }
u16 pop(void) { u16 v = rw(LIN(SG[SS], R[SP])); R[SP] += 2; return v; }

// ---------------- script / keyboard ----------------
static FILE *script;
u16 keyq[64]; int kq_h, kq_t;
static int idle_polls = 0;
static int script_done = 0;
static int shot_n = 0;
static u16 psp_seg;
static void dump_mem(const char *name) {
    char path[1024]; snprintf(path, sizeof path, "%s/%s.mem", outdir, name);
    FILE *f = fopen(path, "wb"); if (f) { fwrite(mem, 1, 0xa0000, f); fclose(f); }
}
void finish(int code) {
#ifdef EMU_LIB
    engine_exited = 1; pthread_exit(NULL);
#endif
    char n[64]; snprintf(n, sizeof n, "final"); save_frame(n); if (getenv("HOOKSTATS")) emu_hook_stats();
    fprintf(stderr, "[emu] exit code %d after %llu instructions, %llu video writes\n", code, icount, vwrites);
    exit(0);
}
int key_avail(void) { return kq_h != kq_t; }
static void key_push(u16 k) { keyq[kq_t] = k; kq_t = (kq_t + 1) & 63; }
#ifdef EMU_LIB
u16 key_pop(void) { pthread_mutex_lock(&kmx); u16 k = keyq[kq_h]; kq_h = (kq_h + 1) & 63; pthread_mutex_unlock(&kmx); return k; }
#else
u16 key_pop(void) { u16 k = keyq[kq_h]; kq_h = (kq_h + 1) & 63; return k; }
#endif
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
// Run script commands until a key is queued. Returns 0 if script ended.
static int script_advance(void) {
    char line[256];
    while (script && fgets(line, sizeof line, script)) {
        char cmd[64] = "", arg[128] = "";
        if (sscanf(line, "%63s %127s", cmd, arg) < 1 || cmd[0] == '#') continue;
        if (!strcmp(cmd, "shot")) { char n[160]; if (arg[0]) snprintf(n, sizeof n, "%s", arg); else snprintf(n, sizeof n, "shot%03d", shot_n); shot_n++; save_frame(n); }
        else if (!strcmp(cmd, "dump")) dump_mem(arg[0] ? arg : "dump");
        else if (!strcmp(cmd, "key")) { u16 k = named_key(arg); key_push(k); return 1; }
        else if (!strcmp(cmd, "text")) { for (char *p = arg; *p; p++) key_push((u8)*p); return 1; }
        else if (!strcmp(cmd, "trace")) trace = atoi(arg);
        else if (!strcmp(cmd, "poke")) { unsigned a, v; if (sscanf(line, "%*s %x %x", &a, &v) == 2) { u32 la = ((u32)(0x0120 + 0x128e) << 4) + a; mem[la] = v & 0xff; mem[la + 1] = v >> 8; } }
        else if (!strcmp(cmd, "quit")) finish(0);
    }
    script_done = 1; return 0;
}
static int idle_limit = 3000;
#ifdef EMU_LIB
void need_key(int blocking) {
    pthread_mutex_lock(&kmx);
    if (blocking) { while (!key_avail() && !engine_quit) pthread_cond_wait(&kcv, &kmx); }
    int have = key_avail(); pthread_mutex_unlock(&kmx);
    if (engine_quit) finish(0);
    if (!have && !blocking) usleep(2000);
}
#else
void need_key(int blocking) {
    if (key_avail()) { idle_polls = 0; return; }
    if (!blocking && ++idle_polls < idle_limit) return;
    idle_polls = 0;
    if (!script_advance()) { fprintf(stderr, "[emu] script ended\n"); finish(0); }
}
#endif

// ---------------- DOS ----------------
#define MAXFH 20
static FILE *fh[MAXFH];
static u16 heap_top;      // next free segment for 48h
static u16 prog_end;      // end of program block
static struct { u16 seg, size; int used; } blocks[64]; static int nblocks;
unsigned long tick = 0;

FILE *open_ci(const char *name, const char *mode) {
    char path[1024]; const char *base = name; const char *p;
    if ((p = strrchr(name, '\\'))) base = p + 1; if ((p = strrchr(base, ':'))) base = p + 1;
    DIR *d; struct dirent *e;
    if (savedir) {
        if (mode[0] == 'w') { snprintf(path, sizeof path, "%s/%s", savedir, base); for (char *q = path + strlen(savedir) + 1; *q; q++) *q = toupper((unsigned char)*q); return fopen(path, mode); }
        d = opendir(savedir);
        if (d) { while ((e = readdir(d))) if (!strcasecmp(e->d_name, base)) { snprintf(path, sizeof path, "%s/%s", savedir, e->d_name); closedir(d); return fopen(path, mode); } closedir(d); }
    }
    d = opendir(gamedir);
    if (d) { while ((e = readdir(d))) if (!strcasecmp(e->d_name, base)) { snprintf(path, sizeof path, "%s/%s", gamedir, e->d_name); closedir(d); return fopen(path, mode); } closedir(d); }
    if (mode[0] == 'w') { snprintf(path, sizeof path, "%s/%s", gamedir, base); return fopen(path, mode); }
    return NULL;
}
static void getstr(u32 a, char *out, int max) { int i = 0; while (i < max - 1) { u8 c = mem[(a + i) & 0xfffff]; if (!c) break; out[i++] = c; } out[i] = 0; }
static void set_cf(int c) { // modify flags in IRET frame on stack
    u32 a = LIN(SG[SS], R[SP] + 4); u16 f = rw(a); f = c ? (f | CF) : (f & ~CF); ww(a, f);
}
static void set_zf_frame(int z) { u32 a = LIN(SG[SS], R[SP] + 4); u16 f = rw(a); f = z ? (f | ZF) : (f & ~ZF); ww(a, f); }

static void dos_int21(void) {
    u8 ah = R[AX] >> 8, al = R[AX] & 0xff; char name[256];
    if (trace) fprintf(stderr, "INT21 AX=%04x BX=%04x CX=%04x DX=%04x DS=%04x ES=%04x\n", R[AX], R[BX], R[CX], R[DX], SG[DS], SG[ES]);
    switch (ah) {
    case 0x02: fputc(R[DX] & 0xff, stderr); break;
    case 0x06: if ((R[DX] & 0xff) == 0xff) { need_key(0); if (key_avail()) { R[AX] = (R[AX] & 0xff00) | (key_pop() & 0xff); set_zf_frame(0); } else { R[AX] &= 0xff00; set_zf_frame(1); } } else fputc(R[DX] & 0xff, stderr); break;
    case 0x07: case 0x08: case 0x01: need_key(1); R[AX] = (R[AX] & 0xff00) | (key_pop() & 0xff); break;
    case 0x09: { u32 a = LIN(SG[DS], R[DX]); while (mem[a] != '$') fputc(mem[a++], stderr); } break;
    case 0x0b: need_key(0); R[AX] = (R[AX] & 0xff00) | (key_avail() ? 0xff : 0); break;
    case 0x0e: R[AX] = (R[AX] & 0xff00) | 3; break;
    case 0x19: R[AX] = (R[AX] & 0xff00) | 2; break;
    case 0x1a: break;
    case 0x25: ww(al * 4, R[DX]); ww(al * 4 + 2, SG[DS]); break;
    case 0x2a: R[CX] = 1990; R[DX] = 0x0101; R[AX] = (R[AX] & 0xff00) | 1; break;
    case 0x2c: {
#ifdef EMU_LIB
        struct timeval tv; gettimeofday(&tv, NULL); unsigned long t = (unsigned long)((tv.tv_sec % 86400) * 100 + tv.tv_usec / 10000);
#else
        tick += 7; unsigned long t = tick;
#endif
        R[CX] = (u16)((((t / 360000) % 24) << 8) | ((t / 6000) % 60)); R[DX] = (u16)((((t / 100) % 60) << 8) | (t % 100)); } break;
    case 0x30: R[AX] = 0x0003; R[BX] = 0; R[CX] = 0; break; // DOS 3.0
    case 0x33: R[DX] &= 0xff00; break;
    case 0x35: R[BX] = rw(al * 4); SG[ES] = rw(al * 4 + 2); break;
    case 0x3b: set_cf(0); break;
    case 0x3c: case 0x3d: case 0x5b: {
        getstr(LIN(SG[DS], R[DX]), name, sizeof name); int h = -1;
        for (int i = 5; i < MAXFH; i++) if (!fh[i]) { h = i; break; }
        FILE *f = NULL;
        if (ah == 0x3d) { f = open_ci(name, (al & 3) == 0 ? "rb" : "r+b"); }
        else f = open_ci(name, "w+b");
        if (trace || 1) fprintf(stderr, "[dos] %s '%s' -> %s\n", ah == 0x3d ? "open" : "create", name, f ? "ok" : "FAIL");
        if (!f || h < 0) { R[AX] = 2; set_cf(1); } else { fh[h] = f; R[AX] = h; set_cf(0); }
    } break;
    case 0x3e: if (R[BX] < MAXFH && fh[R[BX]]) { fclose(fh[R[BX]]); fh[R[BX]] = NULL; } set_cf(0); break;
    case 0x3f: {
        u16 h = R[BX], n = R[CX]; u32 a = LIN(SG[DS], R[DX]);
        if (h == 0) { R[AX] = 0; set_cf(0); break; }
        if (h >= MAXFH || !fh[h]) { R[AX] = 6; set_cf(1); break; }
        u16 got = 0; int c; while (got < n && (c = fgetc(fh[h])) != EOF) { wb(a + got, (u8)c); got++; }
        R[AX] = got; set_cf(0);
    } break;
    case 0x40: {
        u16 h = R[BX], n = R[CX]; u32 a = LIN(SG[DS], R[DX]);
        if (h == 1 || h == 2) { for (u16 i = 0; i < n; i++) fputc(mem[(a + i) & 0xfffff], stderr); R[AX] = n; set_cf(0); break; }
        if (h >= MAXFH || !fh[h]) { R[AX] = 6; set_cf(1); break; }
        for (u16 i = 0; i < n; i++) fputc(mem[(a + i) & 0xfffff], fh[h]);
        R[AX] = n; set_cf(0);
    } break;
    case 0x41: R[AX] = 2; set_cf(1); break;
    case 0x42: {
        u16 h = R[BX]; if (h >= MAXFH || !fh[h]) { R[AX] = 6; set_cf(1); break; }
        s32 off = (s32)(((u32)R[CX] << 16) | R[DX]);
        fseek(fh[h], off, al == 0 ? SEEK_SET : al == 1 ? SEEK_CUR : SEEK_END);
        long p = ftell(fh[h]); R[AX] = p & 0xffff; R[DX] = (p >> 16) & 0xffff; set_cf(0);
    } break;
    case 0x43: set_cf(0); R[CX] = 0x20; break;
    case 0x44: if (al == 0) { R[DX] = R[BX] < 3 ? 0x80d3 : 0x0000; set_cf(0); } else { set_cf(0); } break;
    case 0x47: wb(LIN(SG[DS], R[SI]), 0); set_cf(0); break;
    case 0x48: {
        u16 need = R[BX];
        for (int i = 0; i < nblocks; i++) if (!blocks[i].used && blocks[i].size >= need) { blocks[i].used = 1; R[AX] = blocks[i].seg; set_cf(0); if (trace) fprintf(stderr, "[dos] alloc reuse %04x\n", R[AX]); return; }
        if ((u32)heap_top + need + 1 > 0x9fff) { R[AX] = 8; R[BX] = 0x9fff - heap_top - 1; set_cf(1); break; }
        blocks[nblocks].seg = heap_top; blocks[nblocks].size = need; blocks[nblocks].used = 1; nblocks++;
        R[AX] = heap_top; heap_top += need + 1; set_cf(0);
    } break;
    case 0x49: for (int i = 0; i < nblocks; i++) if (blocks[i].seg == SG[ES]) blocks[i].used = 0; set_cf(0); break;
    case 0x4a: {
        if (SG[ES] == psp_seg) {
            u32 want = (u32)psp_seg + R[BX];
            if (want > 0x9fff) { R[AX] = 8; R[BX] = 0x9fff - psp_seg; set_cf(1); break; }
            prog_end = (u16)want; if (heap_top < prog_end + 1 || nblocks == 0) heap_top = prog_end + 1; set_cf(0);
        } else { int ok = 0; for (int i = 0; i < nblocks; i++) if (blocks[i].seg == SG[ES]) { if (R[BX] <= blocks[i].size || i == nblocks - 1) { if (i == nblocks - 1) heap_top = blocks[i].seg + R[BX] + 1; blocks[i].size = R[BX]; ok = 1; } } if (ok) set_cf(0); else { R[AX] = 8; R[BX] = 0; set_cf(1); } }
    } break;
    case 0x4c: finish(al); break;
    case 0x00: finish(0); break;
    default: fprintf(stderr, "[dos] unhandled INT21 AH=%02x at %04x:%04x\n", ah, rw(LIN(SG[SS], R[SP] + 2)), rw(LIN(SG[SS], R[SP]))); set_cf(1); R[AX] = 1; break;
    }
}
static void host_int(int n) {
    switch (n) {
    case 0x10: {
        u8 ah = R[AX] >> 8;
        if (ah == 0) { set_mode(R[AX] & 0x7f); fprintf(stderr, "[bios] set mode %02x\n", R[AX] & 0xff); }
        else if (ah == 0x0f) { R[AX] = (80 << 8) | video_mode; R[BX] &= 0xff; }
        else if (ah == 0x10 && (R[AX] & 0xff) == 0) { ega_pal[R[BX] & 15] = R[BX] >> 8; }
        else if (ah == 0x12) { if ((R[BX] & 0xff) == 0x10) R[BX] = 0x0003; R[CX] = 0x0009; }
        else if (ah == 0x1a) { R[AX] &= 0xff00; } // no VGA
        else if (trace) fprintf(stderr, "[bios] INT10 AX=%04x\n", R[AX]);
    } break;
    case 0x11: R[AX] = 0x0021; break;
    case 0x12: R[AX] = 640; break;
    case 0x16: {
        u8 ah = R[AX] >> 8;
        if (ah == 0 || ah == 0x10) { need_key(1); R[AX] = key_pop(); }
        else if (ah == 1 || ah == 0x11) { need_key(0); if (key_avail()) { R[AX] = keyq[kq_h]; set_zf_frame(0); } else set_zf_frame(1); }
        else if (ah == 2) R[AX] &= 0xff00;
    } break;
    case 0x1a: { tick += 1; if ((R[AX] >> 8) == 0) { R[CX] = (tick >> 16) & 0xffff; R[DX] = tick & 0xffff; R[AX] &= 0xff00; } } break;
    case 0x21: dos_int21(); break;
    case 0x33: R[AX] = 0; break; // no mouse
    default: if (trace) fprintf(stderr, "[int] unhandled INT %02x\n", n);
    }
}

// ---------------- CPU ----------------
static const u8 parity[256] = {
#define P2(n) n, n^1, n^1, n
#define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
#define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)
    P6(1), P6(0), P6(0), P6(1)
};
static inline void setf(u16 m, int c) { if (c) flags |= m; else flags &= ~m; }
static inline void szp8(u8 v) { setf(ZF, v == 0); setf(SF, v & 0x80); setf(PF, parity[v]); }
static inline void szp16(u16 v) { setf(ZF, v == 0); setf(SF, v & 0x8000); setf(PF, parity[v & 0xff]); }

static int seg_ovr = -1; static int rep = 0;
// modrm decoding
static int m_mod, m_reg, m_rm; static u32 m_ea; static u16 m_off; static int m_seg;
static void modrm(void) {
    u8 b = fetch8(); m_mod = b >> 6; m_reg = (b >> 3) & 7; m_rm = b & 7;
    if (m_mod == 3) return;
    u16 o = 0; int sg = DS;
    switch (m_rm) {
    case 0: o = R[BX] + R[SI]; break; case 1: o = R[BX] + R[DI]; break;
    case 2: o = R[BP] + R[SI]; sg = SS; break; case 3: o = R[BP] + R[DI]; sg = SS; break;
    case 4: o = R[SI]; break; case 5: o = R[DI]; break;
    case 6: if (m_mod == 0) o = fetch16(); else { o = R[BP]; sg = SS; } break;
    case 7: o = R[BX]; break;
    }
    if (m_mod == 1) o += (s8)fetch8(); else if (m_mod == 2) o += fetch16();
    if (seg_ovr >= 0) sg = seg_ovr;
    m_off = o; m_seg = sg; m_ea = LIN(SG[sg], o);
}
static inline u8 *reg8p(int r) { return (r < 4) ? (u8 *)&R[r] : ((u8 *)&R[r - 4]) + 1; } // little-endian host
static inline u8 getr8(int r) { return *reg8p(r); }
static inline void setr8(int r, u8 v) { *reg8p(r) = v; }
static inline u8 rm8(void) { return m_mod == 3 ? getr8(m_rm) : rb(m_ea); }
static inline u16 rm16(void) { return m_mod == 3 ? R[m_rm] : (u16)(rb(m_ea) | (rb(LIN(SG[m_seg], m_off + 1)) << 8)); }
static inline void wrm8(u8 v) { if (m_mod == 3) setr8(m_rm, v); else wb(m_ea, v); }
static inline void wrm16(u16 v) { if (m_mod == 3) R[m_rm] = v; else { wb(m_ea, v & 0xff); wb(LIN(SG[m_seg], m_off + 1), v >> 8); } }

static u8 alu8(int op, u8 a, u8 b) {
    u16 r; int c = (flags & CF) ? 1 : 0;
    switch (op) {
    case 0: r = a + b; setf(CF, r > 0xff); setf(AF, (a ^ b ^ r) & 0x10); setf(OF, (~(a ^ b) & (a ^ r)) & 0x80); break;
    case 1: r = a | b; setf(CF, 0); setf(OF, 0); setf(AF, 0); break;
    case 2: r = a + b + c; setf(CF, r > 0xff); setf(AF, (a ^ b ^ r) & 0x10); setf(OF, (~(a ^ b) & (a ^ r)) & 0x80); break;
    case 3: r = a - b - c; setf(CF, (u16)a < (u16)b + c); setf(AF, (a ^ b ^ r) & 0x10); setf(OF, ((a ^ b) & (a ^ r)) & 0x80); break;
    case 4: r = a & b; setf(CF, 0); setf(OF, 0); setf(AF, 0); break;
    case 5: case 7: r = a - b; setf(CF, a < b); setf(AF, (a ^ b ^ r) & 0x10); setf(OF, ((a ^ b) & (a ^ r)) & 0x80); break;
    default: r = a ^ b; setf(CF, 0); setf(OF, 0); setf(AF, 0); break;
    }
    szp8((u8)r); return (u8)r;
}
static u16 alu16(int op, u16 a, u16 b) {
    u32 r; int c = (flags & CF) ? 1 : 0;
    switch (op) {
    case 0: r = a + b; setf(CF, r > 0xffff); setf(AF, (a ^ b ^ r) & 0x10); setf(OF, (~(a ^ b) & (a ^ r)) & 0x8000); break;
    case 1: r = a | b; setf(CF, 0); setf(OF, 0); setf(AF, 0); break;
    case 2: r = a + b + c; setf(CF, r > 0xffff); setf(AF, (a ^ b ^ r) & 0x10); setf(OF, (~(a ^ b) & (a ^ r)) & 0x8000); break;
    case 3: r = a - b - c; setf(CF, (u32)a < (u32)b + c); setf(AF, (a ^ b ^ r) & 0x10); setf(OF, ((a ^ b) & (a ^ r)) & 0x8000); break;
    case 4: r = a & b; setf(CF, 0); setf(OF, 0); setf(AF, 0); break;
    case 5: case 7: r = a - b; setf(CF, a < b); setf(AF, (a ^ b ^ r) & 0x10); setf(OF, ((a ^ b) & (a ^ r)) & 0x8000); break;
    default: r = a ^ b; setf(CF, 0); setf(OF, 0); setf(AF, 0); break;
    }
    szp16((u16)r); return (u16)r;
}
static u8 inc8(u8 a) { u8 r = a + 1; setf(OF, r == 0x80); setf(AF, (r & 15) == 0); szp8(r); return r; }
static u8 dec8(u8 a) { u8 r = a - 1; setf(OF, r == 0x7f); setf(AF, (r & 15) == 15); szp8(r); return r; }
static u16 inc16(u16 a) { u16 r = a + 1; setf(OF, r == 0x8000); setf(AF, (r & 15) == 0); szp16(r); return r; }
static u16 dec16(u16 a) { u16 r = a - 1; setf(OF, r == 0x7fff); setf(AF, (r & 15) == 15); szp16(r); return r; }

static u16 shift(int op, u16 v, int n, int w) {
    int bits = w ? 16 : 8; u16 msb = w ? 0x8000 : 0x80; u16 maskv = w ? 0xffff : 0xff; int c;
    if (n == 0) return v;
    v &= maskv;
    for (int i = 0; i < n; i++) {
        switch (op) {
        case 0: c = (v & msb) != 0; v = ((v << 1) | c) & maskv; setf(CF, c); break;                        // ROL
        case 1: c = v & 1; v = (v >> 1) | (c ? msb : 0); setf(CF, c); break;                                // ROR
        case 2: c = (v & msb) != 0; v = ((v << 1) | ((flags & CF) ? 1 : 0)) & maskv; setf(CF, c); break;    // RCL
        case 3: c = v & 1; v = (v >> 1) | ((flags & CF) ? msb : 0); setf(CF, c); break;                     // RCR
        case 4: case 6: c = (v & msb) != 0; v = (v << 1) & maskv; setf(CF, c); break;                       // SHL
        case 5: c = v & 1; v >>= 1; setf(CF, c); break;                                                     // SHR
        case 7: c = v & 1; v = (v >> 1) | (v & msb); setf(CF, c); break;                                    // SAR
        }
    }
    (void)bits;
    switch (op) {
    case 0: case 2: case 4: case 6: setf(OF, ((v & msb) != 0) ^ ((flags & CF) != 0)); break;
    case 1: case 3: setf(OF, ((v & msb) != 0) ^ ((v & (msb >> 1)) != 0)); break;
    case 5: setf(OF, n == 1 ? ((v & (msb >> 1)) != 0) : 0); break;
    case 7: setf(OF, 0); break;
    }
    if (op >= 4) { if (w) szp16(v); else szp8((u8)v); }
    return v;
}
static void do_int(int n) {
    push(flags); push(SG[CS]); push(ip);
    flags &= ~(IF | TF);
    ip = rw(n * 4); SG[CS] = rw(n * 4 + 2);
}
static int cond(int c) {
    int r;
    switch (c >> 1) {
    case 0: r = flags & OF; break; case 1: r = flags & CF; break; case 2: r = flags & ZF; break;
    case 3: r = flags & (CF | ZF); break; case 4: r = flags & SF; break; case 5: r = flags & PF; break;
    case 6: r = ((flags & SF) != 0) != ((flags & OF) != 0); break;
    default: r = (flags & ZF) || (((flags & SF) != 0) != ((flags & OF) != 0)); break;
    }
    return (c & 1) ? !r : !!r;
}
static void unimpl(u8 op) {
    fprintf(stderr, "[emu] unimplemented opcode %02x at %04x:%04x (icount %llu)\n", op, SG[CS], (u16)(ip - 1), icount);
    save_frame("crash"); exit(2);
}
static void string_op(u8 op) {
    int w = op & 1; int d = (flags & DF) ? -1 : 1; if (w) d *= 2;
    int sseg = seg_ovr >= 0 ? seg_ovr : DS;
    for (;;) {
        if (rep && R[CX] == 0) break;
        switch (op & 0xfe) {
        case 0xa4: if (w) ww(LIN(SG[ES], R[DI]), rw(LIN(SG[sseg], R[SI]))); else wb(LIN(SG[ES], R[DI]), rb(LIN(SG[sseg], R[SI]))); R[SI] += d; R[DI] += d; break;
        case 0xa6: if (w) alu16(7, rw(LIN(SG[sseg], R[SI])), rw(LIN(SG[ES], R[DI]))); else alu8(7, rb(LIN(SG[sseg], R[SI])), rb(LIN(SG[ES], R[DI]))); R[SI] += d; R[DI] += d; break;
        case 0xaa: if (w) ww(LIN(SG[ES], R[DI]), R[AX]); else wb(LIN(SG[ES], R[DI]), R[AX] & 0xff); R[DI] += d; break;
        case 0xac: if (w) R[AX] = rw(LIN(SG[sseg], R[SI])); else setr8(0, rb(LIN(SG[sseg], R[SI]))); R[SI] += d; break;
        case 0xae: if (w) alu16(7, R[AX], rw(LIN(SG[ES], R[DI]))); else alu8(7, R[AX] & 0xff, rb(LIN(SG[ES], R[DI]))); R[DI] += d; break;
        }
        if (!rep) break;
        R[CX]--;
        if ((op & 0xfe) == 0xa6 || (op & 0xfe) == 0xae) { if (rep == 1 && !(flags & ZF)) break; if (rep == 2 && (flags & ZF)) break; }
    }
}
// ---- native hooks: ported functions replace original code at their entry points ----
typedef void (*native_fn)(void);
#define MAXHOOK 512
static struct { u32 lin; native_fn fn; int far_ret; const char *name; unsigned long calls; } hooktab[MAXHOOK]; static int nhooks;
static u8 hookflag[0x100000];
u16 load_seg = 0x0120;
int hooks_enabled = 1;
void emu_hook(u16 gseg, u16 off, native_fn fn, int far_ret, const char *name) {
    u32 lin = ((u32)(gseg - 0x1000 + load_seg) << 4) + off;
    hooktab[nhooks].lin = lin; hooktab[nhooks].fn = fn; hooktab[nhooks].far_ret = far_ret; hooktab[nhooks].name = name; nhooks++; hookflag[lin] = 1;
}
void emu_hook_stats(void) { for (int i = 0; i < nhooks; i++) fprintf(stderr, "[hook] %-28s %lu calls\n", hooktab[i].name, hooktab[i].calls); }
static int run_hook(u32 lin) {
    for (int i = 0; i < nhooks; i++) if (hooktab[i].lin == lin) {
        hooktab[i].calls++; hooktab[i].fn();
        if (hooktab[i].far_ret) { ip = pop(); SG[CS] = pop(); } else ip = pop();
        return 1;
    }
    return 0;
}
void step(void);
// Call original (still emulated) code from native code. Far cdecl: args pushed right to left, caller cleans.
u16 emu_call_far(u16 gseg, u16 off, int nargs, ...) {
    va_list ap; u16 args[16]; va_start(ap, nargs); for (int i = 0; i < nargs; i++) args[i] = (u16)va_arg(ap, int); va_end(ap);
    for (int i = nargs - 1; i >= 0; i--) push(args[i]);
    u16 save_cs = SG[CS], save_ip = ip; u16 sp0 = R[SP];
    push(0xf000); push(0x0800); SG[CS] = gseg - 0x1000 + load_seg; ip = off;
    while (!(SG[CS] == 0xf000 && ip == 0x0800 && R[SP] == sp0)) step();
    SG[CS] = save_cs; ip = save_ip; R[SP] += nargs * 2;
    return R[AX];
}
static int residue_trace = -1;
void step(void) {
    seg_ovr = -1; rep = 0; u8 op;
    if (residue_trace < 0) residue_trace = getenv("RESIDUE_TRACE") != NULL;
    if (residue_trace && ip == 0x3138 && SG[CS] == 0x0120) {      // entry of country_turn in the original
        u32 sp = LIN(SG[SS], R[SP]);
        fprintf(stderr, "[residue] p=%d rel=%d r=%d lastwriter=%04x:%04x\n", (s16)rw(sp + 4), (s16)rw(sp - 0x30), (s16)rw(sp - 0x2c), (u16)(watch_cs - 0x0120 + 0x1000), watch_ip);
        watch_addr = sp - 0x2c + 1;
    }
    if (residue_trace && ip == 0x4675 && SG[CS] == 0x0120) { u32 sp = LIN(SG[SS], R[SP]); fprintf(stderr, "[residue] briefing choice=%d\n", (s16)rw(sp - 0xb8)); }
    if (residue_trace && ip == 0x9376 && SG[CS] == 0x0120) { u32 sp = LIN(SG[SS], R[SP]); fprintf(stderr, "[residue] arms count=%d\n", (s16)rw(sp - 0x1c)); }
    if (hooks_enabled) { u32 lin = LIN(SG[CS], ip); if (lin < 0x100000 && hookflag[lin] && run_hook(lin)) return; }
    for (;;) { // prefixes
        op = fetch8();
        if (op == 0x26) seg_ovr = ES; else if (op == 0x2e) seg_ovr = CS; else if (op == 0x36) seg_ovr = SS; else if (op == 0x3e) seg_ovr = DS;
        else if (op == 0xf3) rep = 1; else if (op == 0xf2) rep = 2; else if (op == 0xf0) ; else break;
    }
    icount++;
    if (op < 0x40 && (op & 7) < 6) { // ALU
        int a = op >> 3; u16 x, y;
        switch (op & 7) {
        case 0: modrm(); x = alu8(a, rm8(), getr8(m_reg)); if (a != 7) wrm8((u8)x); break;
        case 1: modrm(); x = alu16(a, rm16(), R[m_reg]); if (a != 7) wrm16(x); break;
        case 2: modrm(); x = alu8(a, getr8(m_reg), rm8()); if (a != 7) setr8(m_reg, (u8)x); break;
        case 3: modrm(); x = alu16(a, R[m_reg], rm16()); if (a != 7) R[m_reg] = x; break;
        case 4: y = fetch8(); x = alu8(a, R[AX] & 0xff, (u8)y); if (a != 7) setr8(0, (u8)x); break;
        case 5: y = fetch16(); x = alu16(a, R[AX], y); if (a != 7) R[AX] = x; break;
        }
        return;
    }
    switch (op) {
    case 0x06: push(SG[ES]); break; case 0x07: SG[ES] = pop(); break;
    case 0x0e: push(SG[CS]); break;
    case 0x16: push(SG[SS]); break; case 0x17: SG[SS] = pop(); break;
    case 0x1e: push(SG[DS]); break; case 0x1f: SG[DS] = pop(); break;
    case 0x27: { u8 al = R[AX] & 0xff; int cf = flags & CF; if ((al & 15) > 9 || (flags & AF)) { al += 6; setf(AF, 1); } else setf(AF, 0); if ((R[AX] & 0xff) > 0x99 || cf) { al += 0x60; setf(CF, 1); } else setf(CF, 0); setr8(0, al); szp8(al); } break;
    case 0x2f: { u8 al = R[AX] & 0xff, old = al; int cf = flags & CF; if ((al & 15) > 9 || (flags & AF)) { al -= 6; setf(AF, 1); } else setf(AF, 0); if (old > 0x99 || cf) { al -= 0x60; setf(CF, 1); } else setf(CF, 0); setr8(0, al); szp8(al); } break;
    case 0x37: if ((R[AX] & 15) > 9 || (flags & AF)) { R[AX] += 0x106; setf(AF, 1); setf(CF, 1); } else { setf(AF, 0); setf(CF, 0); } R[AX] &= 0xff0f; break;
    case 0x3f: if ((R[AX] & 15) > 9 || (flags & AF)) { R[AX] -= 6; R[AX] -= 0x100; setf(AF, 1); setf(CF, 1); } else { setf(AF, 0); setf(CF, 0); } R[AX] &= 0xff0f; break;
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: R[op & 7] = inc16(R[op & 7]); break;
    case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: R[op & 7] = dec16(R[op & 7]); break;
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x55: case 0x56: case 0x57: push(R[op & 7]); break;
    case 0x54: push(R[SP] - 2); break; // 8086 behaviour
    case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: R[op & 7] = pop(); break;
    case 0x60: { u16 sp = R[SP]; push(R[AX]); push(R[CX]); push(R[DX]); push(R[BX]); push(sp); push(R[BP]); push(R[SI]); push(R[DI]); } break;
    case 0x61: R[DI] = pop(); R[SI] = pop(); R[BP] = pop(); pop(); R[BX] = pop(); R[DX] = pop(); R[CX] = pop(); R[AX] = pop(); break;
    case 0x68: push(fetch16()); break;
    case 0x6a: push((u16)(s8)fetch8()); break;
    case 0x69: { modrm(); s32 r = (s32)(s16)rm16() * (s16)fetch16(); R[m_reg] = (u16)r; setf(CF | OF, r != (s16)r); } break;
    case 0x6b: { modrm(); s16 v = (s16)rm16(); s32 r = (s32)v * (s8)fetch8(); R[m_reg] = (u16)r; setf(CF | OF, r != (s16)r); } break;
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
    case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: { s8 d = (s8)fetch8(); if (cond(op & 15)) ip += d; } break;
    case 0x80: case 0x82: { modrm(); u8 a = rm8(); u8 b = fetch8(); u8 r = alu8(m_reg, a, b); if (m_reg != 7) wrm8(r); } break;
    case 0x81: { modrm(); u16 a = rm16(); u16 b = fetch16(); u16 r = alu16(m_reg, a, b); if (m_reg != 7) wrm16(r); } break;
    case 0x83: { modrm(); u16 a = rm16(); u16 b = (u16)(s8)fetch8(); u16 r = alu16(m_reg, a, b); if (m_reg != 7) wrm16(r); } break;
    case 0x84: modrm(); alu8(4, rm8(), getr8(m_reg)); break;
    case 0x85: modrm(); alu16(4, rm16(), R[m_reg]); break;
    case 0x86: { modrm(); u8 t = rm8(); wrm8(getr8(m_reg)); setr8(m_reg, t); } break;
    case 0x87: { modrm(); u16 t = rm16(); wrm16(R[m_reg]); R[m_reg] = t; } break;
    case 0x88: modrm(); wrm8(getr8(m_reg)); break;
    case 0x89: modrm(); wrm16(R[m_reg]); break;
    case 0x8a: modrm(); setr8(m_reg, rm8()); break;
    case 0x8b: modrm(); R[m_reg] = rm16(); break;
    case 0x8c: modrm(); wrm16(SG[m_reg & 3]); break;
    case 0x8d: modrm(); R[m_reg] = m_off; break;
    case 0x8e: modrm(); SG[m_reg & 3] = rm16(); break;
    case 0x8f: { modrm(); u16 v = pop(); wrm16(v); } break;
    case 0x90: break;
    case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: { u16 t = R[AX]; R[AX] = R[op & 7]; R[op & 7] = t; } break;
    case 0x98: R[AX] = (u16)(s8)(R[AX] & 0xff); break;
    case 0x99: R[DX] = (R[AX] & 0x8000) ? 0xffff : 0; break;
    case 0x9a: { u16 o = fetch16(), s = fetch16(); push(SG[CS]); push(ip); ip = o; SG[CS] = s; } break;
    case 0x9b: break;
    case 0x9c: push(flags | 0xf002); break;
    case 0x9d: flags = (pop() & 0x0fd5) | 2; break;
    case 0x9e: flags = (flags & 0xff00) | ((R[AX] >> 8) & 0xd5) | 2; break;
    case 0x9f: setr8(4, (u8)(flags & 0xff)); break;
    case 0xa0: { u16 o = fetch16(); setr8(0, rb(LIN(SG[seg_ovr >= 0 ? seg_ovr : DS], o))); } break;
    case 0xa1: { u16 o = fetch16(); R[AX] = rw(LIN(SG[seg_ovr >= 0 ? seg_ovr : DS], o)); } break;
    case 0xa2: { u16 o = fetch16(); wb(LIN(SG[seg_ovr >= 0 ? seg_ovr : DS], o), R[AX] & 0xff); } break;
    case 0xa3: { u16 o = fetch16(); ww(LIN(SG[seg_ovr >= 0 ? seg_ovr : DS], o), R[AX]); } break;
    case 0xa4: case 0xa5: case 0xa6: case 0xa7: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf: string_op(op); break;
    case 0xa8: alu8(4, R[AX] & 0xff, fetch8()); break;
    case 0xa9: alu16(4, R[AX], fetch16()); break;
    case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7: setr8(op & 7, fetch8()); break;
    case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf: R[op & 7] = fetch16(); break;
    case 0xc0: { modrm(); u8 v = rm8(); int n = fetch8() & 31; wrm8((u8)shift(m_reg, v, n, 0)); } break;
    case 0xc1: { modrm(); u16 v = rm16(); int n = fetch8() & 31; wrm16(shift(m_reg, v, n, 1)); } break;
    case 0xc2: { u16 n = fetch16(); ip = pop(); R[SP] += n; } break;
    case 0xc3: ip = pop(); break;
    case 0xc4: case 0xc5: { modrm(); u16 o = rw(m_ea), s = rw(LIN(SG[m_seg], m_off + 2)); R[m_reg] = o; SG[op == 0xc4 ? ES : DS] = s; } break;
    case 0xc6: { modrm(); wrm8(fetch8()); } break;
    case 0xc7: { modrm(); wrm16(fetch16()); } break;
    case 0xc8: { u16 n = fetch16(); fetch8(); push(R[BP]); R[BP] = R[SP]; R[SP] -= n; } break;
    case 0xc9: R[SP] = R[BP]; R[BP] = pop(); break;
    case 0xca: { u16 n = fetch16(); ip = pop(); SG[CS] = pop(); R[SP] += n; } break;
    case 0xcb: ip = pop(); SG[CS] = pop(); break;
    case 0xcc: do_int(3); break;
    case 0xcd: { u8 n = fetch8(); do_int(n); } break;
    case 0xce: if (flags & OF) do_int(4); break;
    case 0xcf: ip = pop(); SG[CS] = pop(); flags = (pop() & 0x0fd5) | 2; break;
    case 0xd0: { modrm(); wrm8((u8)shift(m_reg, rm8(), 1, 0)); } break;
    case 0xd1: { modrm(); wrm16(shift(m_reg, rm16(), 1, 1)); } break;
    case 0xd2: { modrm(); wrm8((u8)shift(m_reg, rm8(), R[CX] & 0xff, 0)); } break;
    case 0xd3: { modrm(); wrm16(shift(m_reg, rm16(), R[CX] & 0xff, 1)); } break;
    case 0xd4: { u8 b = fetch8(); u8 al = R[AX] & 0xff; if (!b) { do_int(0); break; } setr8(4, al / b); setr8(0, al % b); szp8(R[AX] & 0xff); } break;
    case 0xd5: { u8 b = fetch8(); u8 al = (u8)((R[AX] & 0xff) + (R[AX] >> 8) * b); R[AX] = al; szp8(al); } break;
    case 0xd7: setr8(0, rb(LIN(SG[seg_ovr >= 0 ? seg_ovr : DS], (u16)(R[BX] + (R[AX] & 0xff))))); break;
    case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf: modrm(); break; // FPU ignored
    case 0xe0: { s8 d = (s8)fetch8(); if (--R[CX] && !(flags & ZF)) ip += d; } break;
    case 0xe1: { s8 d = (s8)fetch8(); if (--R[CX] && (flags & ZF)) ip += d; } break;
    case 0xe2: { s8 d = (s8)fetch8(); if (--R[CX]) ip += d; } break;
    case 0xe3: { s8 d = (s8)fetch8(); if (!R[CX]) ip += d; } break;
    case 0xe4: setr8(0, port_in(fetch8())); break;
    case 0xe5: { u8 p = fetch8(); R[AX] = port_in(p) | (port_in(p + 1) << 8); } break;
    case 0xe6: port_out(fetch8(), R[AX] & 0xff); break;
    case 0xe7: { u8 p = fetch8(); port_out(p, R[AX] & 0xff); port_out(p + 1, R[AX] >> 8); } break;
    case 0xe8: { u16 d = fetch16(); push(ip); ip += d; } break;
    case 0xe9: { u16 d = fetch16(); ip += d; } break;
    case 0xea: { u16 o = fetch16(), s = fetch16(); ip = o; SG[CS] = s; } break;
    case 0xeb: { s8 d = (s8)fetch8(); ip += d; } break;
    case 0xec: setr8(0, port_in(R[DX])); break;
    case 0xed: R[AX] = port_in(R[DX]) | (port_in(R[DX] + 1) << 8); break;
    case 0xee: port_out(R[DX], R[AX] & 0xff); break;
    case 0xef: port_out(R[DX], R[AX] & 0xff); port_out(R[DX] + 1, R[AX] >> 8); break;
    case 0xf4: fprintf(stderr, "[emu] HLT\n"); finish(0); break;
    case 0xf5: flags ^= CF; break;
    case 0xf6: {
        modrm(); u8 v = rm8();
        switch (m_reg) {
        case 0: case 1: alu8(4, v, fetch8()); break;
        case 2: wrm8(~v); break;
        case 3: { u8 r = alu8(5, 0, v); setf(CF, v != 0); wrm8(r); } break;
        case 4: { u16 r = (u16)(R[AX] & 0xff) * v; R[AX] = r; setf(CF | OF, r > 0xff); } break;
        case 5: { s16 r = (s16)(s8)(R[AX] & 0xff) * (s8)v; R[AX] = (u16)r; setf(CF | OF, r != (s8)r); } break;
        case 6: { if (!v) { do_int(0); break; } u16 a = R[AX]; u16 q = a / v; if (q > 0xff) { do_int(0); break; } setr8(0, (u8)q); setr8(4, (u8)(a % v)); } break;
        case 7: { if (!v) { do_int(0); break; } s16 a = (s16)R[AX]; s16 q = a / (s8)v; if (q > 127 || q < -128) { do_int(0); break; } setr8(0, (u8)q); setr8(4, (u8)(a % (s8)v)); } break;
        }
    } break;
    case 0xf7: {
        modrm(); u16 v = rm16();
        switch (m_reg) {
        case 0: case 1: alu16(4, v, fetch16()); break;
        case 2: wrm16(~v); break;
        case 3: { u16 r = alu16(5, 0, v); setf(CF, v != 0); wrm16(r); } break;
        case 4: { u32 r = (u32)R[AX] * v; R[AX] = (u16)r; R[DX] = r >> 16; setf(CF | OF, R[DX] != 0); } break;
        case 5: { s32 r = (s32)(s16)R[AX] * (s16)v; R[AX] = (u16)r; R[DX] = (u16)(r >> 16); setf(CF | OF, r != (s16)r); } break;
        case 6: { if (!v) { do_int(0); break; } u32 a = ((u32)R[DX] << 16) | R[AX]; u32 q = a / v; if (q > 0xffff) { do_int(0); break; } R[AX] = (u16)q; R[DX] = (u16)(a % v); } break;
        case 7: { if (!v) { do_int(0); break; } s32 a = (s32)(((u32)R[DX] << 16) | R[AX]); s32 q = a / (s16)v; if (q > 32767 || q < -32768) { do_int(0); break; } R[AX] = (u16)q; R[DX] = (u16)(a % (s16)v); } break;
        }
    } break;
    case 0xf8: flags &= ~CF; break; case 0xf9: flags |= CF; break;
    case 0xfa: flags &= ~IF; break; case 0xfb: flags |= IF; break;
    case 0xfc: flags &= ~DF; break; case 0xfd: flags |= DF; break;
    case 0xfe: { modrm(); u8 v = rm8(); if (m_reg == 0) wrm8(inc8(v)); else if (m_reg == 1) wrm8(dec8(v)); else unimpl(op); } break;
    case 0xff: {
        modrm();
        switch (m_reg) {
        case 0: wrm16(inc16(rm16())); break;
        case 1: wrm16(dec16(rm16())); break;
        case 2: { u16 t = rm16(); push(ip); ip = t; } break;
        case 3: { u16 o = rw(m_ea), s = rw(LIN(SG[m_seg], m_off + 2)); push(SG[CS]); push(ip); ip = o; SG[CS] = s; } break;
        case 4: ip = rm16(); break;
        case 5: { u16 o = rw(m_ea), s = rw(LIN(SG[m_seg], m_off + 2)); ip = o; SG[CS] = s; } break;
        case 6: push(rm16()); break;
        default: unimpl(op);
        }
    } break;
    case 0x0f: { // host trap: 0F FF nn
        u8 b = fetch8(); if (b != 0xff) unimpl(op); u8 n = fetch8(); host_int(n);
    } break;
    default: unimpl(op);
    }
}

void port_register_hooks(void) __attribute__((weak));
static void load_exe(const char *path) {
    FILE *f = fopen(path, "rb"); if (!f) { perror(path); exit(1); }
    static u8 buf[0x100000]; size_t n = fread(buf, 1, sizeof buf, f); fclose(f);
    u16 *h = (u16 *)buf; if (h[0] != 0x5a4d) { fprintf(stderr, "not MZ\n"); exit(1); }
    u32 hdr = h[4] * 16; u32 img = (h[2] - 1) * 512 + (h[1] ? h[1] : 512) - hdr; if (hdr + img > n) img = (u32)n - hdr;
    u16 env_seg = 0x0100; psp_seg = 0x0110; u16 load = psp_seg + 0x10;
    memcpy(mem + load * 16, buf + hdr, img);
    for (int i = 0; i < h[3]; i++) { u16 o = *(u16 *)(buf + h[12] + i * 4), s = *(u16 *)(buf + h[12] + i * 4 + 2); u32 a = (load + s) * 16 + o; u16 v = mem[a] | (mem[a + 1] << 8); v += load; mem[a] = v & 0xff; mem[a + 1] = v >> 8; }
    // environment
    u8 *e = mem + env_seg * 16; const char *envs = "PATH=C:\\"; strcpy((char *)e, envs); e += strlen(envs) + 1; *e++ = 0; *e++ = 1; *e++ = 0; strcpy((char *)e, "C:\\CONFLICT.EXE");
    // PSP
    u8 *p = mem + psp_seg * 16; p[0] = 0xcd; p[1] = 0x20; p[2] = 0xff; p[3] = 0x9f; p[0x2c] = env_seg & 0xff; p[0x2d] = env_seg >> 8; p[0x80] = 0; p[0x81] = 0x0d;
    prog_end = 0x9fff; heap_top = 0x9fff;
    SG[DS] = SG[ES] = psp_seg; SG[SS] = load + h[7]; R[SP] = h[8]; SG[CS] = load + h[11]; ip = h[10];
    if (port_register_hooks && hooks_enabled) port_register_hooks();
    fprintf(stderr, "[emu] loaded %s: image %u bytes at %04x:0000, entry %04x:%04x, ss:sp %04x:%04x\n", path, img, load, SG[CS], ip, SG[SS], R[SP]);
}
#ifdef EMU_LIB
static void cpu_init(void) {
    for (int n = 0; n < 256; n++) { u32 a = 0xf0000 + n * 4; mem[a] = 0x0f; mem[a + 1] = 0xff; mem[a + 2] = n; mem[a + 3] = 0xcf; mem[n * 4] = (n * 4) & 0xff; mem[n * 4 + 1] = (n * 4) >> 8; mem[n * 4 + 2] = 0x00; mem[n * 4 + 3] = 0xf0; }
    mem[0x449] = 3; mem[0x44a] = 80; mem[0x484] = 24; mem[0x487] = 0x60; mem[0x410] = 0x21;
    set_mode(3);
}
static char g_exe[1024], g_game[1024], g_save[1024];
static void *engine_thread(void *arg) { (void)arg; cpu_init(); load_exe(g_exe); for (;;) { step(); if (engine_quit) finish(0); } return NULL; }
void engine_start(const char *game_dir, const char *save_dir) {
    snprintf(g_game, sizeof g_game, "%s", game_dir); snprintf(g_save, sizeof g_save, "%s", save_dir); snprintf(g_exe, sizeof g_exe, "%s/Conflict.exe", game_dir);
    gamedir = g_game; savedir = g_save; pthread_t t; pthread_create(&t, NULL, engine_thread, NULL); pthread_detach(t);
}
void engine_post_key(unsigned short k) { pthread_mutex_lock(&kmx); if (((kq_t + 1) & 63) != kq_h) key_push(k); pthread_cond_signal(&kcv); pthread_mutex_unlock(&kmx); }
int engine_has_exited(void) { return engine_exited; }
void engine_request_quit(void) { engine_quit = 1; pthread_mutex_lock(&kmx); pthread_cond_signal(&kcv); pthread_mutex_unlock(&kmx); }
// Fills a 640x350 BGRA buffer. Returns 1 when a graphics mode is active.
int engine_copy_frame(unsigned char *bgra) {
    static unsigned int pal[16]; 
    for (int c = 0; c < 16; c++) { u8 e = ega_pal[c]; u8 r = ((e >> 2) & 1) * 0xaa + ((e >> 5) & 1) * 0x55, g = ((e >> 1) & 1) * 0xaa + ((e >> 4) & 1) * 0x55, b = ((e >> 0) & 1) * 0xaa + ((e >> 3) & 1) * 0x55; pal[c] = 0xff000000u | (r << 16) | (g << 8) | b; }
    unsigned int *out = (unsigned int *)bgra;
    if (video_mode != 0x10) { for (int i = 0; i < 640 * 350; i++) out[i] = 0xff000000u; return 0; }
    for (int y = 0; y < 350; y++) for (int xb = 0; xb < 80; xb++) {
        u32 off = y * 80 + xb; u8 p0 = plane[0][off], p1 = plane[1][off], p2 = plane[2][off], p3 = plane[3][off];
        for (int bit = 7; bit >= 0; bit--) { int c = ((p0 >> bit) & 1) | (((p1 >> bit) & 1) << 1) | (((p2 >> bit) & 1) << 2) | (((p3 >> bit) & 1) << 3); *out++ = pal[c]; }
    }
    return 1;
}
#else
int main(int argc, char **argv) {
    const char *exe = NULL, *scr = NULL; unsigned long long maxi = 0; const char *calltest = NULL; u16 ct[32]; int nct = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d")) gamedir = argv[++i]; else if (!strcmp(argv[i], "-o")) outdir = argv[++i];
        else if (!strcmp(argv[i], "-s")) scr = argv[++i]; else if (!strcmp(argv[i], "-t")) trace = 1;
        else if (!strcmp(argv[i], "-H")) hooks_enabled = 0;
        else if (!strcmp(argv[i], "-c")) { calltest = argv[++i]; while (i + 1 < argc && nct < 32 && argv[i + 1][0] != '/' && strncmp(argv[i + 1], "run", 3)) ct[nct++] = (u16)strtol(argv[++i], 0, 16); }
        else if (!strcmp(argv[i], "-n")) maxi = strtoull(argv[++i], 0, 10); else if (!strcmp(argv[i], "-i")) idle_limit = atoi(argv[++i]);
        else exe = argv[i];
    }
    if (getenv("FLTTEST")) {
        for (int n = 0; n < 256; n++) { u32 a = 0xf0000 + n * 4; mem[a] = 0x0f; mem[a + 1] = 0xff; mem[a + 2] = n; mem[a + 3] = 0xcf; mem[n * 4] = (n * 4) & 0xff; mem[n * 4 + 1] = (n * 4) >> 8; mem[n * 4 + 2] = 0x00; mem[n * 4 + 3] = 0xf0; }
        hooks_enabled = 0; load_exe(exe);
        while (icount < 300000) step();     // let the C runtime start-up run first
        static u8 snap[0xa0000]; memcpy(snap, mem, sizeof snap); u16 ss0 = SG[SS], sp0 = R[SP];
        char op[16]; unsigned long long a, b;
        while (scanf("%15s %llx %llx", op, &a, &b) == 3) {
            memcpy(mem, snap, sizeof snap); SG[SS] = ss0; R[SP] = sp0 - 0x200; SG[DS] = 0x13ae; SG[ES] = 0x13ae; icount = 0; flags &= ~DF;
            unsigned off = !strcmp(op, "mul") ? 0x4d6 : !strcmp(op, "div") ? 0x390 : !strcmp(op, "add") ? 0x1b9 : !strcmp(op, "sub") ? 0x167 : !strcmp(op, "d2i") ? 0x747 : !strcmp(op, "i2d") ? 0x686 : !strcmp(op, "d2f") ? 0x7f6 : !strcmp(op, "f2d") ? 0x6fa : 0x32f;
            int unary = !strcmp(op, "d2i") || !strcmp(op, "i2d") || !strcmp(op, "d2f") || !strcmp(op, "f2d");
            // a on stack (first operand), b in registers
            if (!unary) { push((u16)(a >> 48)); push((u16)(a >> 32)); push((u16)(a >> 16)); push((u16)a); }
            else b = a;
            if (!strcmp(op, "i2d")) { R[AX] = (u16)b; }
            else if (!strcmp(op, "f2d")) { R[DX] = (u16)(b >> 16); R[AX] = (u16)b; }
            else { R[AX] = (u16)(b >> 48); R[BX] = (u16)(b >> 32); R[CX] = (u16)(b >> 16); R[DX] = (u16)b; }
            mem[0xf0800] = 0xf4; push(0xf000); push(0x0800); SG[CS] = 0x2007 - 0x1000 + 0x0120; ip = off;
            for (;;) { if (SG[CS] == 0xf000 && ip == 0x0800) break; step(); if (icount > 1000000) { printf("runaway\n"); break; } }
            if (!strcmp(op, "d2i")) printf("%04x\n", R[AX]);
            else if (!strcmp(op, "d2f")) printf("%04x%04x\n", R[DX], R[AX]);
            else if (!strcmp(op, "cmp")) printf("%d\n", (flags & ZF) ? 0 : (((flags & SF) != 0) != ((flags & OF) != 0)) ? -1 : 1);
            else printf("%04x%04x%04x%04x\n", R[AX], R[BX], R[CX], R[DX]);
        }
        return 0;
    }
    if (calltest) {
        for (int n = 0; n < 256; n++) { u32 a = 0xf0000 + n * 4; mem[a] = 0x0f; mem[a + 1] = 0xff; mem[a + 2] = n; mem[a + 3] = 0xcf; mem[n * 4] = (n * 4) & 0xff; mem[n * 4 + 1] = (n * 4) >> 8; mem[n * 4 + 2] = 0x00; mem[n * 4 + 3] = 0xf0; }
        load_exe(exe);
        unsigned sg, of; sscanf(calltest, "%x:%x", &sg, &of);
        SG[DS] = 0x0120 + 0x1b02; // DGROUP guess not needed for math
        R[AX] = ct[0]; R[BX] = ct[1]; R[CX] = ct[2]; R[DX] = ct[3];
        for (int i = nct - 1; i >= 4; i--) push(ct[i]);
        mem[0xf0800] = 0xf4; push(0xf000); push(0x0800);
        SG[CS] = sg - 0x1000 + 0x0120; ip = of;
        for (;;) { if (SG[CS] == 0xf000 && ip == 0x0800) break; step(); if (icount > 1000000) { printf("runaway\n"); break; } }
        printf("AX=%04x BX=%04x CX=%04x DX=%04x flags=%04x (CF=%d ZF=%d SF=%d) SPdelta=%d\n", R[AX], R[BX], R[CX], R[DX], flags, !!(flags & CF), !!(flags & ZF), !!(flags & SF), 0);
        union { unsigned long long u; double d; } v; v.u = ((unsigned long long)R[AX] << 48) | ((unsigned long long)R[BX] << 32) | ((unsigned long long)R[CX] << 16) | R[DX];
        printf("as double: %g ; as int AX=%d ; long DX:AX=%ld\n", v.d, (s16)R[AX], (long)(s32)(((u32)R[DX] << 16) | R[AX]));
        return 0;
    }
    if (!exe) { fprintf(stderr, "usage: emu [-d gamedir] [-o outdir] [-s script] [-n maxinstr] exe\n"); return 1; }
    if (scr) script = fopen(scr, "r");
    // IVT -> host trap stubs at F000:n*4
    for (int n = 0; n < 256; n++) { u32 a = 0xf0000 + n * 4; mem[a] = 0x0f; mem[a + 1] = 0xff; mem[a + 2] = n; mem[a + 3] = 0xcf; mem[n * 4] = (n * 4) & 0xff; mem[n * 4 + 1] = (n * 4) >> 8; mem[n * 4 + 2] = 0x00; mem[n * 4 + 3] = 0xf0; }
    mem[0x449] = 3; mem[0x44a] = 80; mem[0x484] = 24; mem[0x487] = 0x60; mem[0x410] = 0x21;
    set_mode(3);
    load_exe(exe);
    for (;;) {
        if (trace > 1) fprintf(stderr, "%04x:%04x AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x SP=%04x DS=%04x ES=%04x F=%04x\n", SG[CS], ip, R[AX], R[BX], R[CX], R[DX], R[SI], R[DI], R[BP], R[SP], SG[DS], SG[ES], flags);
        step();
        if (maxi && icount >= maxi) { fprintf(stderr, "[emu] instruction limit at %04x:%04x\n", SG[CS], ip); finish(0); }
    }
}
#endif
