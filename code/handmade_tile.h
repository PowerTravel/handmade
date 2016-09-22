#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#include "handmade.h"

struct tile_map_position{

	// Position within a tile
	// Measured from center
	real32 X;
	real32 Y;
	real32 Z;

	// Note: The high bits are the tile page index
	// 		 The low bits are the tile index relative to the page
	uint32 AbsTileX;
	uint32 AbsTileY;
	uint32 AbsTileZ;
};

// Tile Chunk Position is teh Chunk Index from tile_map_position
// separated into its parts of high bits and low bits
struct tile_index{
	// High bits of ChunkIndex
	uint32 PageX;
	uint32 PageY;
	uint32 PageZ;

	// Low bits of page index, Each page only has x/y coords
	uint32 TileX;
	uint32 TileY;
};

struct tile_pages{
	uint32* Page;
};

struct tile_map{
	// 	meters
	real32 TileSide;

	uint32 PageShift;
	uint32 PageMask;

	uint32 PageDim;

	uint32 PageCountX;
	uint32 PageCountY;	
	uint32 PageCountZ;

	tile_pages* Map;
};
#endif // HANDMADE_TILE_H