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


#define OBJ_MAX_LINE_LENGTH 512
#define OBJ_MAX_WORD_LENGTH 64


#if 0
struct component_list
{
	u32 Type;
	void* Data;
	component_list* Next;
};

struct entity
{
	u32 Index;
	component_list* Component;
};

struct world_entities
{
	entity Entities[32];
	u32 NrEntities;
};

enum component_type
{
	CAMERA  		= 0x1,
	COLLISION 		= 0x2,
	GEOMETRY  		= 0x4
};

struct geometry_component
{
	obj_geometry O;
	m4 M;
};

#endif
struct loaded_bitmap
{
	s32   Width;
	s32   Height;
	void* Pixels;
};

class RootNode;
class CameraNode;
class TransformNode;

struct game_state{

	memory_arena AssetArena;
	memory_arena TemporaryArena;
	r32 t;

//	RootNode* Root;
//	CameraNode* Camera;

	loaded_bitmap testBMP;
	obj_geometry  testOBJ;

	world World;

	b32 IsInitialized;
};

#endif // HANDMADE_H
