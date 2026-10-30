// Engine interface shared by the Swift front end and the game core.
#ifndef CONFLICT_ENGINE_H
#define CONFLICT_ENGINE_H
#define ENGINE_WIDTH 640
#define ENGINE_HEIGHT 350
void engine_start(const char *game_dir, const char *save_dir);
void engine_post_key(unsigned short bios_key);   // high byte scancode, low byte ASCII
int engine_copy_frame(unsigned char *bgra);      // 640x350 BGRA; returns 1 if a frame is available
int engine_has_exited(void);
void engine_request_quit(void);
#endif
