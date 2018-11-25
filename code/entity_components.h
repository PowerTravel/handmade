#ifndef ENTITY_COMPONENTS_H
#define ENTITY_COMPONENTS_H

static s32 EntityID = 0;

enum component_types
{
	COMPONENT_TYPE_EMPTY        = 0x00,
	COMPONENT_TYPE_POSITION  	= 0x01,
	COMPONENT_TYPE_MESH	 		= 0x02,
	COMPONENT_TYPE_CAMERA 		= 0x04,
	COMPONENT_TYPE_CONTROLLER   = 0x08

};

struct triangle{
	s32 vi[3];
	s32 vni[3];
	v4 n;
};

struct face
{
	s32 nv;	// Nr Vertices
	s32* vi; 	// Vertex Indeces
	s32* ni;	// Normal Indeces
	s32* ti;  // Texture Indeces
};

struct obj_geometry
{
	s32 nv;
	v4* v;

	s32 nvn;
	v4* vn;

	s32 nvt;
	v3* vt;

	s32   nf;
	face* f;

	s32 nt;
	triangle* t;
};

struct geometry_component
{
	m4 T;
	obj_geometry* Object;
};

struct camera_component
{
	r32 AngleOfView;
	r32 ScreenHeight;
	r32 ScreenWidth;
	m4  DeltaRot;
	v3  DeltaPos;
	m4  V;
	m4  P;
};

struct position_component
{
	v3 Position;
};

struct controller_component
{
	game_controller_input* Controller;
};

struct entity
{
	u32 id;
	u32 Types;
	geometry_component 	  GeometryComponent;
	camera_component	  CameraComponent;
	position_component    PositionComponent;
	controller_component  ControllerComponent;
};

struct world
{
	entity WorldEntities[10];	
};

entity CreateBlankEntity()
{
	entity Result ={};
	Result.id = EntityID++;
	return Result;
};



//void InitializeWorldEntities(world_entities* WorldEntities )
//{
//	TileMap->PageShift = 4;
//	TileMap->PageMask = (1<<TileMap->PageShift)-1;		
//	TileMap->PageDim = (1<<TileMap->PageShift);
//
//	for(u32 TilePageIndex = 0;
//		TilePageIndex < ArrayCount(TileMap->MapHash);
//		++TilePageIndex)
//	{
//		TileMap->MapHash[TilePageIndex].PageX = TILE_PAGE_UNINITIALIZED;
//	}
//}


#endif // ENTITY_COMPONENT_H