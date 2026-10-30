// Engine interface for the Windows front end, backed by the native port.
// Same contract as port/engine_native.c, using Win32 threads and timing.
#include <windows.h>
#include <stdlib.h>
#include "../port/conflict.h"
#include "../port/ega.h"

int platform_load_data(const char *path);

static char game_dir[MAX_PATH], save_dir[MAX_PATH];
static CRITICAL_SECTION key_lock;
static CONDITION_VARIABLE key_cond;
static u16 keyq[64]; static int kq_h, kq_t;
static volatile LONG exited, quit_requested;
static int clock_calls, fake_clock, idle_polls;

static void leave(void) { InterlockedExchange(&exited, 1); ExitThread(0); }
void host_exit(void) { leave(); }

int host_key_available(void) {
    if (quit_requested) leave();
    EnterCriticalSection(&key_lock);
    int have = kq_h != kq_t;
    LeaveCriticalSection(&key_lock);
    // Paces the original's polling loops at roughly 180 microseconds per poll:
    // Windows sleeps in whole milliseconds, so sleep 1 ms on every sixth empty poll.
    if (!have && ++idle_polls >= 6) { idle_polls = 0; Sleep(1); }
    return have;
}

u16 host_key_read(void) {
    EnterCriticalSection(&key_lock);
    while (kq_h == kq_t && !quit_requested) SleepConditionVariableCS(&key_cond, &key_lock, INFINITE);
    if (quit_requested) { LeaveCriticalSection(&key_lock); leave(); }
    u16 k = keyq[kq_h]; kq_h = (kq_h + 1) & 63;
    LeaveCriticalSection(&key_lock);
    return k;
}

// First call seeds the random generator from the real clock; later calls answer the
// delay calibration so that the delay factor comes out as exactly 1.
int host_clock_cs(void) {
    if (clock_calls++ == 0) {
        SYSTEMTIME st; GetLocalTime(&st);
        fake_clock = ((st.wHour * 60 + st.wMinute) * 60 + st.wSecond) * 100 + st.wMilliseconds / 10;
        return (int)(i16)(u16)fake_clock;
    }
    fake_clock += 17;
    return (int)(i16)(u16)fake_clock;
}

void host_delay_ms(int ms) { if (quit_requested) leave(); if (ms > 0) Sleep((DWORD)ms); }

FILE *host_fopen(const char *name, const char *mode) {
    char path[MAX_PATH * 2];
    snprintf(path, sizeof path, "%s\\%s", save_dir, name);
    if (mode[0] == 'w') return fopen(path, mode);           // saved games
    FILE *f = fopen(path, mode);
    if (f) return f;
    snprintf(path, sizeof path, "%s\\%s", game_dir, name);
    return fopen(path, mode);
}

static DWORD WINAPI game_thread(LPVOID arg) { (void)arg; game_main(); InterlockedExchange(&exited, 1); return 0; }

int engine_start(const char *game, const char *save) {
    char data[MAX_PATH * 2];
    InitializeCriticalSection(&key_lock);
    InitializeConditionVariable(&key_cond);
    snprintf(game_dir, sizeof game_dir, "%s", game);
    snprintf(save_dir, sizeof save_dir, "%s", save);
    snprintf(data, sizeof data, "%s\\conflict.dat", game);
    if (platform_load_data(data) != 0) return -1;
    ega_set_mode(3);
    gfx_plain_keys = 1;
    timeBeginPeriod(1);
    HANDLE t = CreateThread(NULL, 8 << 20, game_thread, NULL, 0, NULL);
    if (!t) return -1;
    CloseHandle(t);
    return 0;
}
void engine_post_key(unsigned short k) {
    EnterCriticalSection(&key_lock);
    if (((kq_t + 1) & 63) != kq_h) { keyq[kq_t] = k; kq_t = (kq_t + 1) & 63; }
    LeaveCriticalSection(&key_lock);
    WakeConditionVariable(&key_cond);
}
int engine_copy_frame(unsigned char *bgra) { ega_to_bgra((uint32_t *)bgra); return ega_mode == 0x10; }
int engine_has_exited(void) { return exited != 0; }
void engine_request_quit(void) { InterlockedExchange(&quit_requested, 1); WakeAllConditionVariable(&key_cond); }
