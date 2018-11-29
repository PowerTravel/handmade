#ifndef HANDMADE_H
#define HANDMADE_H

#include "platform.h"
#include "intrinsics.h"
#include "vector_math.h"
#include "affine_transformations.h"
#include "string.h"
#include "shared.h"
#include "memory.h"
#include "entity_components.h"
#include "materials.h"
#include "data_containers.h"
#include "render.h"

#define OBJ_MAX_LINE_LENGTH 512
#define OBJ_MAX_WORD_LENGTH 64

struct game_state{

	memory_arena AssetArena;
	memory_arena TemporaryArena;
	depth_buffer DepthBuffer;

	r32 t;

	loaded_bitmap testBMP;
	obj_geometry  testOBJ0;
	obj_geometry  testOBJ1;
	obj_geometry  testOBJ2;
	obj_geometry  testOBJ3;

	world* World;

	b32 IsInitialized;
};

#endif // HANDMADE_H
