#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"	
#include "handmade_math.h"
#include "handmade_tile.h"
#include "handmade_intrinsics.h"

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
	
	bool32 JustMoved;
	bool32 MovedUp;
	memory_arena WorldArena;
	world* World;
	tile_map_position PlayerPos;
	loaded_bitmap Backdrop;
	loaded_bitmap Head;
	loaded_bitmap Body;
	
};



#endif // HANDMADE_H
