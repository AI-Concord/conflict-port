// Engine interface for the Mac front end, backed by the native port.
// The game runs on its own thread; the UI thread posts keys and reads the frame.
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <strings.h>
#include <sys/time.h>
#include "conflict.h"
#include "ega.h"

int platform_load_data(const char *path);

static char game_dir[1024], save_dir[1024];
static pthread_mutex_t key_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_cond = PTHREAD_COND_INITIALIZER;
static u16 keyq[64]; static int kq_h, kq_t;
static volatile int exited, quit_requested;
static int clock_calls; static int fake_clock;

static void leave(void) { exited = 1; pthread_exit(NULL); }
void host_exit(void) { leave(); }

int host_key_available(void) {
    if (quit_requested) leave();
    pthread_mutex_lock(&key_mutex);
    int have = kq_h != kq_t;
    pthread_mutex_unlock(&key_mutex);
    if (!have) usleep(180);                       // paces the original's polling loops (see maps.c delay_or_key)
    return have;
}

u16 host_key_read(void) {
    pthread_mutex_lock(&key_mutex);
    while (kq_h == kq_t && !quit_requested) pthread_cond_wait(&key_cond, &key_mutex);
    if (quit_requested) { pthread_mutex_unlock(&key_mutex); leave(); }
    u16 k = keyq[kq_h]; kq_h = (kq_h + 1) & 63;
    pthread_mutex_unlock(&key_mutex);
    return k;
}

// First call seeds the random generator from the real clock. Later calls come from the
// delay calibration, which is answered so that the delay factor comes out as exactly 1.
int host_clock_cs(void) {
    if (clock_calls++ == 0) {
        struct timeval tv; gettimeofday(&tv, NULL);
        fake_clock = (int)((tv.tv_sec % 86400) * 100 + tv.tv_usec / 10000);
        return (int)(i16)(u16)fake_clock;
    }
    fake_clock += 17;
    return (int)(i16)(u16)fake_clock;
}

void host_delay_ms(int ms) { if (quit_requested) leave(); if (ms > 0) usleep((useconds_t)ms * 1000); }

static FILE *open_in(const char *dir, const char *name, const char *mode) {
    char path[2048]; DIR *d = opendir(dir); struct dirent *e;
    if (!d) return NULL;
    while ((e = readdir(d))) if (!strcasecmp(e->d_name, name)) { snprintf(path, sizeof path, "%s/%s", dir, e->d_name); closedir(d); return fopen(path, mode); }
    closedir(d);
    return NULL;
}
FILE *host_fopen(const char *name, const char *mode) {
    char path[2048];
    if (mode[0] == 'w') { snprintf(path, sizeof path, "%s/%s", save_dir, name); return fopen(path, mode); }   // saved games
    FILE *f = open_in(save_dir, name, mode);
    return f ? f : open_in(game_dir, name, mode);
}

static void *game_thread(void *arg) { (void)arg; game_main(); exited = 1; return NULL; }

void engine_start(const char *game, const char *save) {
    char data[1100];
    snprintf(game_dir, sizeof game_dir, "%s", game); snprintf(save_dir, sizeof save_dir, "%s", save);
    snprintf(data, sizeof data, "%s/conflict.dat", game);
    if (platform_load_data(data) != 0) { exited = 1; return; }
    ega_set_mode(3);
    gfx_plain_keys = 1;
    pthread_t t; pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 8 << 20);
    pthread_create(&t, &a, game_thread, NULL); pthread_detach(t);
}
void engine_post_key(unsigned short k) {
    pthread_mutex_lock(&key_mutex);
    if (((kq_t + 1) & 63) != kq_h) { keyq[kq_t] = k; kq_t = (kq_t + 1) & 63; }
    pthread_cond_signal(&key_cond);
    pthread_mutex_unlock(&key_mutex);
}
int engine_copy_frame(unsigned char *bgra) { ega_to_bgra((uint32_t *)bgra); return ega_mode == 0x10; }
int engine_has_exited(void) { return exited; }
void engine_request_quit(void) { quit_requested = 1; pthread_mutex_lock(&key_mutex); pthread_cond_broadcast(&key_cond); pthread_mutex_unlock(&key_mutex); }
