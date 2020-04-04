#pragma once
#include "platform.h"
#include "debug.h"
#include "memory.h"
#include "bitmap.h"
#include "handmade_tile.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "externals/stb_truetype.h"

struct contact_data;
struct entity;

union v2s32
{
  struct{
     s32 X, Y;
  };
   s32 E[2];
};

v2s32 V2S32(s32 i, s32 j)
{
  v2s32 Result = {};
  Result.X = i;
  Result.Y = j;
  return Result;
}

struct stb_font_map
{
  s32 StartChar;
  s32 NumChars;
  r32 FontHeightPx;

  r32 Ascent;
  r32 Descent;
  r32 LineGap;

  bitmap BitMap;
  stbtt_bakedchar* CharData;
};


struct game_assets
{
  sprite_sheet TileMapSpriteSheet;
  sprite_sheet HeroSpriteSheet;
  stb_font_map STBFontMap;
};

struct contact_manifold;

struct world
{
  r32 GlobalTimeSec;
  r32 dtForFrame;

  memory_arena* AssetArena;
  memory_arena* PersistentArena;
  memory_arena* TransientArena;

  tile_map     TileMap;
  u32          NrEntities;
  u32          NrMaxEntities;
  entity*      Entities;

  u32 MaxNrManifolds;
  contact_manifold* Manifolds;
  contact_manifold* FirstContactManifold;

  game_assets*  Assets;
};

struct game_state
{
  memory_arena AssetArena;
  memory_arena PersistentArena;
  memory_arena TransientArena;
  world* World;
  b32 IsInitialized;
};