#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "shared.hpp"
#include "memory.hpp"

struct loaded_bitmap
{
	int32 Width;
	int32 Height;
	void* Pixels;
};

struct game_state{

	memory_arena AssetArena;

	real32 t;

	loaded_bitmap testBMP;

	bool32 IsInitialized;
};

#endif // HANDMADE_H
