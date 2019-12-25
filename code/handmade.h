#ifndef HANDMADE_H
#define HANDMADE_H

#include <stdio.h>

#include "platform.h"

#include "vector_math.h"

#include "memory.h"
#include "data_containers.h"
#include "bitmap.h"
#include "entity_components.h"
#include "render_push_buffer.h"
#include "handmade_tile.h"
#include "gjk.h"

struct game_assets
{
	sprite_sheet TileMapSpriteSheet;
	sprite_sheet HeroSpriteSheet;
};

struct world
{
	r32 GlobalTimeSec;
	r32 dtForFrame;

	memory_arena 	Arena;

	tile_map		TileMap;
	u32 			NrEntities;
	u32 			NrMaxEntities;
	entity* 		Entities;
	game_assets*	Assets;
};

struct game_state
{
	memory_arena AssetArena;
	memory_arena TemporaryArena;

	r32 t;

	world* World;

	b32 IsInitialized;
};

#endif // HANDMADE_H
