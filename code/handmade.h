#pragma once


#include "platform.h"
#include "debug.h"
#include "memory.h"
#include "bitmap.h"
#include "handmade_tile.h"
#include "dynamic_aabb_tree.h"
#include "assets.h"
#include "entity_components.h"
#include "epa_collision_data.h"

struct world
{
  r32 GlobalTimeSec;
  r32 dtForFrame;

  memory_arena* Arena;

  tile_map     TileMap;

  aabb_tree BroadPhaseTree;
  world_contact_chunk* ContactManifolds;
};

// This is to be a ginormous struct where we can set things we wanna access from everywhere.
struct game_state
{
  r32 ScreenWidthPixels;
  r32 ScreenHeightPixels;

  memory_arena* PersistentArena;
  memory_arena* TransientArena;
  temporary_memory TransientTempMem;

  game_asset_manager* AssetManager;
  entity_manager* EntityManager;

  world* World;

  b32 IsInitialized;
};