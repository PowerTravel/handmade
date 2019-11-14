#ifndef ENTITY_COMPONENTS_H
#define ENTITY_COMPONENTS_H

#include "vector_math.h"
#include "handmade_tile.h"
#include "memory.h"

static s32 EntityID = 0;

#include "component_camera.h"
#include "component_controller.h"
#include "component_light.h"
#include "component_mesh.h"
#include "component_spatial.h"
#include "component_surface.h"
#include "component_sprite_animation.h"

struct entity
{
	u32 id;
	u32 Types;

	component_camera*			CameraComponent;
	component_controller*  		ControllerComponent;
	component_light*			LightComponent;
	component_mesh* 		 	MeshComponent;
	component_spatial*	 		SpatialComponent;
	component_collision*	 	CollisionComponent;
	component_dynamics*	 		DynamicsComponent;
	component_surface*  		SurfaceComponent;
	component_sprite_animation* SpriteAnimationComponent;

};

struct game_assets
{
	component_sprite_animation* PrinnySet;

	u32 NrEdgeTiles;
	floor_tile_sprite* EdgeTiles;

	u32 NrFloorTiles;
	floor_tile_sprite* FloorTiles;
};

struct world
{
	r32 GlobalTimeSec;
	r32 dtForFrame;

	memory_arena 	Arena;

	tile_map		TileMap;
	u32 			NrEntities;
	u32 			NrMaxEntities;
	entity* 		Entities;
	game_assets*	Assets;
};

enum component_types
{
	COMPONENT_TYPE_EMPTY        	 = 0x0000,
	COMPONENT_TYPE_CAMERA 			 = 0x0001,
	COMPONENT_TYPE_LIGHT   		     = 0x0002,
	COMPONENT_TYPE_CONTROLLER   	 = 0x0004,
	COMPONENT_TYPE_MESH 	  		 = 0x0008,
	COMPONENT_TYPE_SPATIAL			 = 0x0010,
	COMPONENT_TYPE_COLLISION		 = 0x0020,
	COMPONENT_TYPE_DYNAMICS			 = 0x0040,
	COMPONENT_TYPE_SURFACE 		 	 = 0x0080,
	COMPONENT_TYPE_SPRITE_ANIMATION  = 0x0100,
	COMPONENT_TYPE_FINAL 			 = 0x0200
};


#endif // ENTITY_COMPONENT_H