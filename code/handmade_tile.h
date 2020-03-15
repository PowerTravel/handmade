#pragma once

#include "types.h"
#include "math/vector_math.h"
#include "data_containers.h"
#include "math/aabb.h"

struct bitmap;

struct tile_map_position{

  // Position within a tile
  // Measured from lower left
  r32 RelTileX;
  r32 RelTileY;
  r32 RelTileZ;

  // Note: The high bits are the tile page index
  //       The low bits are the tile index relative to the page
  u32 AbsTileX;
  u32 AbsTileY;
  u32 AbsTileZ;
};

// Tile Chunk Position is teh Chunk Index from tile_map_position
// separated into its parts of high bits and low bits
struct tile_index{
  // High bits of PageIndex
  s32 PageX;
  s32 PageY;
  s32 PageZ;

  // Low bits of page index, Each page only has x/y coords
  u32 TileX;
  u32 TileY;
};

enum tile_type
{
  TILE_TYPE_NONE,
  TILE_TYPE_FLOOR,
  TILE_TYPE_WALL
};

struct tile_contents
{
  tile_type Type;
  bitmap* Bitmap;
  m4 TM;
};

struct tile_page{
  s32 PageX;
  s32 PageY;
  s32 PageZ;

  tile_contents* Page;

  tile_page* NextInHash;
};

struct tile_map{
  r32 TileHeightInMeters;
  r32 TileWidthInMeters;
  r32 TileDepthInMeters;

  s32 PageShift;
  s32 PageMask;
  s32 PageDim;

  // NOTE(Jakob): At the moment this needs to be a power of 2
  tile_page MapHash[4096];
};


void InitializeTileMap( tile_map* TileMap );
void GetIntersectingTiles(tile_map* TileMap, list<tile_map_position>* OutputList, aabb3f* AABB );
inline tile_contents GetTileContents(tile_map* TileMap, tile_page* TilePage, s32 RelTileIndexX, s32 RelTileIndexY );
inline tile_contents GetTileContents(tile_map* TileMap, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ);
inline tile_contents GetTileContents(tile_map* TileMap, tile_map_position CanPos);
aabb3f GetTileAABB(tile_map* TileMap, tile_map_position CanPos );