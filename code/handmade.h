#ifndef HANDMADE_H
#define HANDMADE_H

#include "types.h"
#include "platform.h"
#include "random.h"
#include "memory.h"
#include "intrinsics.h"
#include "vector_math.h"
#include "affine_transformations.h"
#include "string.h"
#include "data_containers.h"
#include "entity_components.h"
#include "obj_loader.h"
#include "materials.h"
#include "render.h"

struct game_state
{
	memory_arena AssetArena;
	memory_arena TemporaryArena;
	depth_buffer DepthBuffer;

	r32 t;

	bitmap testBMP;

	world* World;

	b32 IsInitialized;
};

#endif // HANDMADE_H
