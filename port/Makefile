# Hybrid test build: reference emulator + ported functions hooked in.
PORT_SRC = ega.c gfx.c ui.c paper.c ai.c maps.c briefing.c war.c diplomacy.c fronts.c border.c arms.c policy.c strikes.c warsim.c news.c hooks.c
CFLAGS = -O2 -w
GAME_SRC = ega.c gfx.c ui.c paper.c ai.c maps.c briefing.c war.c diplomacy.c fronts.c border.c arms.c policy.c strikes.c warsim.c news.c
all: emu_h conflict_headless
emu_h: ../re/emu/emu.c $(PORT_SRC) conflict.h hybrid.h ega.h
	cc $(CFLAGS) -o emu_h ../re/emu/emu.c $(PORT_SRC)
clean:
	rm -f emu_h
conflict_headless: $(GAME_SRC) platform.c headless.c conflict.h ega.h
	cc $(CFLAGS) -o conflict_headless $(GAME_SRC) platform.c headless.c
