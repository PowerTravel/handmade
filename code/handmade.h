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

struct contact_data;

struct game_assets
{
	sprite_sheet TileMapSpriteSheet;
	sprite_sheet HeroSpriteSheet;
};

struct contact_data_list
{
	entity* A;
	entity* B;

  u32 MaxNrContacts = 0;
	u32 NrContacts  = 0;
	contact_data* Contacts;
};

struct world
{
	r32 GlobalTimeSec;
	r32 dtForFrame;

	memory_arena PersistentArena;
	memory_arena* TransientArena;

	tile_map		 TileMap;
	u32 			   NrEntities;
	u32 			   NrMaxEntities;
	entity* 		 Entities;

	u32 NrContacts;
	u32 MaxNrContacts;
	contact_data_list* Contacts;

	game_assets*	Assets;
};

struct game_state
{
	memory_arena AssetArena;
	memory_arena TemporaryArena;
	world* World;

	b32 IsInitialized;
};

#endif // HANDMADE_H
