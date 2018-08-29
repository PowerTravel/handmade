#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"
#include "handmade_tile.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"

struct memory_arena
{
	memory_index Size;
	uint8* Base;
	memory_index Used;
};


internal void
InitializeArena(memory_arena* Arena, memory_index Size, uint8* Base)
{
	Arena->Size = Size;
	Arena->Base = Base;
	Arena->Used = 0;
}

#define PushStruct(Arena, type) (type*) PushSize_(Arena, sizeof(type))

#define PushArray(Arena, Count, type) (type*) PushSize_(Arena, (Count)*sizeof(type))

void*
PushSize_(memory_arena* Arena, memory_index Size)
{
	Assert( (Arena->Used+Size) <= Arena->Size );
	void* Result = Arena->Base + Arena->Used;	
	Arena->Used += Size;
	return Result; 
}

struct loaded_bitmap
{
	int32 Width;
	int32 Height;
	void* Pixels;
};

struct world
{	
	tile_map* TileMap;
};

struct game_state{
		
	v3 CursorPosition;

	memory_arena WorldArena;
	world* World;

	loaded_bitmap CursorBMP;

};



#endif // HANDMADE_H
