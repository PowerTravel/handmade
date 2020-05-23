#pragma once

#include "platform.h"
#include "debug.h"
#include "memory.h"
#include "bitmap.h"
#include "handmade_tile.h"

#include "assets.h"


struct entity;
struct contact_manifold;

struct world
{
  r32 GlobalTimeSec;
  r32 dtForFrame;

  memory_arena* Arena;

  tile_map     TileMap;
  u32          NrEntities;
  u32          NrMaxEntities;
  entity*      Entities;

  u32 MaxNrManifolds;
  contact_manifold* Manifolds;
  contact_manifold* FirstContactManifold;
};

// This is to be a ginormous struct where we can set things
// we wanna access from everywhere. (Except the debug system which is it's own thing)
struct game_state
{
  r32 ScreenWidthPixels;
  r32 ScreenHeightPixels;

  memory_arena* PersistentArena;
  memory_arena* TransientArena;

  temporary_memory TransientTempMem;

  game_asset_manager* AssetManager;

  world* World;

  b32 IsInitialized;
};