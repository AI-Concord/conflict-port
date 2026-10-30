// Glue for the hybrid build: ported functions run natively inside the reference
// emulator, replacing original routines at their entry points.
#ifndef HYBRID_H
#define HYBRID_H
#include "conflict.h"
extern u8 mem[]; extern u16 R[8], SG[4]; extern u16 load_seg;
typedef void (*native_fn)(void);
void emu_hook(u16 gseg, u16 off, native_fn fn, int far_ret, const char *name);
u16 emu_call_far(u16 gseg, u16 off, int nargs, ...);
static inline u16 stk(int n) { u32 a = ((u32)SG[2] << 4) + (u16)(R[4] + 4 + 2 * n); return (u16)(mem[a] | (mem[a + 1] << 8)); }
#define A(n)  ((i16)stk(n))          // nth word argument of the far call being replaced
#define AS(n) S(stk(n))              // nth argument as a string / near pointer
#define RET(v) (R[0] = (u16)(v))
#define HOOK(seg, off, fn) emu_hook(0x##seg, 0x##off, h_##fn, 1, #fn)
#endif
