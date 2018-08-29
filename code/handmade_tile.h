#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#include "handmade_math.h"

struct tile_map_position{

	// Position within a tile
	// Measured from center
	v3 RelTile;

	// Note: The high bits are the tile page index
	// 		 The low bits are the tile index relative to the page
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;
};

// Tile Chunk Position is teh Chunk Index from tile_map_position
// separated into its parts of high bits and low bits
struct tile_index{
	// High bits of PageIndex
	int32 PageX;
	int32 PageY;
	int32 PageZ;

	// Low bits of page index, Each page only has x/y coords
	uint32 TileX;
	uint32 TileY;
};

struct tile_page{
	int32 PageX;
	int32 PageY;
	int32 PageZ;

	uint32* Page;

	tile_page* NextInHash;
};

struct tile_map{
	real32 TileSideInMeters;

	int32 PageShift;
	int32 PageMask;
	int32 PageDim;

	// NOTE(Jakob): At the moment this needs to be a power of 2
	tile_page MapHash[4096];
};
#endif // HANDMADE_TILE_H
