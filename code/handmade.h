#ifndef HANDMADE_H
#define HANDMADE_H

#include <stdio.h>

#include "types.h"
#include "platform.h"
#include "memory.h"
#include "render_push_buffer.h"
#include "handmade_tile.h"


struct game_state
{
	memory_arena AssetArena;
	memory_arena TemporaryArena;

	r32 t;

	world* World;

	b32 IsInitialized;
};

#endif // HANDMADE_H
