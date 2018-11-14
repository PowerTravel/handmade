#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "string.h"
#include "shared.h"
#include "memory.h"
#include "geometric_objects.h"



#define OBJ_MAX_LINE_LENGTH 512
#define OBJ_MAX_WORD_LENGTH 64

struct triangle{
	int32 vi[3];
	v4 n;
};

struct face{
	int32 nv;	// Nr Vertices
	int32* vi; 	// Vertex Indeces
	int32* ni;	// Normal Indeces
	int32* ti;  // Texture Indeces
};


struct obj_geometry
{
	int32   nv;
	v4* v;

	int32   nvn;
	v4* vn;

	int32 nvt;
	v3* vt;

	int32   nf;
	face*  f;

	int32 nt;
	triangle* t;
};


struct loaded_bitmap
{
	int32 Width;
	int32 Height;
	void* Pixels;
};

struct game_state{

	memory_arena AssetArena;
	memory_arena TemporaryArena;
	real32 t;


	loaded_bitmap testBMP;
	obj_geometry  testOBJ;

	bool32 IsInitialized;
};

#endif // HANDMADE_H
