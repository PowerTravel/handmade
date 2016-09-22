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
#include "handmade_tile.h"

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

#include "handmade_intrinsics.h"
//#include "handmade_tile.cpp"

struct 	memory_arena
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


inline game_controller_input* GetController(game_input* Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input* Result  = &Input->Controllers[ControllerIndex];
	return Result;
}

struct game_state{
	
	memory_arena WorldArena;
	world* World;
	tile_map_position PlayerPos;
	loaded_bitmap Backdrop;
	loaded_bitmap Head;
	loaded_bitmap Body;
	
};



#endif // HANDMADE_H
