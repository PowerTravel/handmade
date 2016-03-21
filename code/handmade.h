#ifndef HANDMADE_H
#define HANDMADE_H

/*
	NOTE: 
	 
	HANDMADE_INTERNAL
		0: build for public release 
		1: build for developer only

	HANDMADE_SLOW
		0: no slow code allowed
		1: slow code welcome
*/


#include <stdint.h>

#include "handmade_platform.h"	


#define internal		 static
#define local_persist    static
#define global_variable  static

#define Pi32 3.14159265359




#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif // HANDMADE_SLOW

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terrabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) ( sizeof(Array)/sizeof((Array)[0]) )


inline uint32 
SafeTruncateUInt64(uint64 Value)
{
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return Result;  
}

struct canonical_position{

	// Tile pos in pixels
	real32 RelTileX;
	real32 RelTileY;

	// Tile on a tile map
	int32 TileX;
	int32 TileY;

	// Tile map on the world
	int32 TileMapX;
	int32 TileMapY;
};

struct raw_position{

	real32 TileMapPosX;
	real32 TileMapPosY;

	int32 TileMapX;
	int32 TileMapY;
};
struct tile_map{
	uint32* Tiles;
};

struct world
{
	// Fundamental Meter to Pixel scale
	uint32 TileSideInPixels;
	real32 TileSideInMeters;

	//real32 PixelsPerMeter;

	int32 TileMapWidth;
	int32 TileMapHeight;

	int32 Width;
	int32 Height;

	tile_map* TileMaps;
};


inline game_controller_input* GetController(game_input* Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input* Result  = &Input->Controllers[ControllerIndex];
	return Result;
}

struct game_state{
	
	raw_position PlayerPos;

};



#endif // HANDMADE_H
