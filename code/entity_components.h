#ifndef ENTITY_COMPONENTS_H
#define ENTITY_COMPONENTS_H

static s32 EntityID = 0;

struct component_camera
{
	r32 AngleOfView;
	m4  DeltaRot;
	v3  DeltaPos;
	m4  V;
	m4  P;
	m4  R;
};

struct component_controller
{
	game_controller_input* Controller;
};

struct component_light
{
	v4 Color;
	v4 Position;
//	v4 Direction;
//	r32 Brightness;
//	r32 ConeAngle;
};

struct bitmap
{
	u32   Width;
	u32   Height;
	void* Pixels;
};

struct material
{
	v4 AmbientColor;
	v4 DiffuseColor;
	v4 SpecularColor;
	r32 Shininess;
};

struct surface_property
{
	material* Material;
	bitmap*   DiffuseMap;
};

struct face
{
	u32  nv;	// Nr Vertices
	u32* vi; 	// Vertex Indeces
	u32* ni;	// Normal Indeces
	u32* ti;  	// Texture Indeces
};

struct mesh_data
{
	u32 nv;		 // Nr Verices
	v4* v;		 // Vertices

	u32 nvn; 	 // Nr Vertice Normals
	v4* vn;		 // Vertice Normals

	u32 nvt; 	 // Nr Trxture Vertices 
	v3* vt; 	 // Texture Vertices
};


// Note (Jakob): component_render_mesh should contain all things needed for rendering a mesh. 
// 				 That Means Material, Triangles, Textures of all kinds, diffuse, normal, bump etc.
// 			     component_render_mesh is only used for rendering purposes and can only have 1
// 				 Surface property each.

struct component_render_mesh
{
	m4 T;

	// Geometric object
	u32   TriangleCount;
	face* Triangles;

	// How the surface should be rendered.
	surface_property SurfaceProperty;


	mesh_data* Data;
};

struct component_collidable_rect
{
	v2 Position;
	v2 Dimensions;
};

struct entity
{
	u32 id;
	u32 Types;

	component_camera*			CameraComponent;
	component_controller*  		ControllerComponent;
	component_light*			LightComponent;
	component_render_mesh* 		RenderMeshComponent;
	component_collidable_rect*  CollidableRectComponent;
};

struct world
{
	u32 			NrEntities;
	u32 			NrMaxEntities;
	entity* 		Entities;
	memory_arena 	Arena;
};


enum component_types
{
	COMPONENT_TYPE_EMPTY        	= 0x00,
	COMPONENT_TYPE_CAMERA 			= 0x01,
	COMPONENT_TYPE_LIGHT   			= 0x02,
	COMPONENT_TYPE_CONTROLLER   	= 0x04,
	COMPONENT_TYPE_RENDER_MESH 	  	= 0x08,
	COMPONENT_TYPE_COLLIDABLE_RECT  = 0x20, 
	COMPONENT_TYPE_FINAL 			= 0x40
};

world* AllocateWorld( u32 NrMaxEntities )
{
	memory_arena WorldArena = {};
	world* Result = (world*) PushStruct( &WorldArena, world );
	Copy(sizeof(memory_arena), &WorldArena, &Result->Arena);
	Assert(WorldArena.CurrentBlock == Result->Arena.CurrentBlock);

	Result->NrEntities = 0;
	Result->NrMaxEntities = NrMaxEntities;
	Result->Entities = (entity*) PushArray( &Result->Arena, NrMaxEntities, entity );

	return Result;
}

void NewComponents( world* World, entity* Entity, u32 EntityFlags )
{
	Assert( EntityFlags )
	Assert( ! ( EntityFlags & ( ~(COMPONENT_TYPE_FINAL - 1) ) ) )

	Entity->Types = Entity->Types | EntityFlags;

	if( EntityFlags & COMPONENT_TYPE_CAMERA )
	{
		Entity->CameraComponent = (component_camera*) PushStruct(&World->Arena, component_camera);
	}
	if( EntityFlags & COMPONENT_TYPE_CONTROLLER )
	{
		Entity->ControllerComponent = (component_controller*) PushStruct(&World->Arena, component_controller);
	}
	if( EntityFlags & COMPONENT_TYPE_RENDER_MESH )
	{
		Entity->RenderMeshComponent = (component_render_mesh*) PushStruct(&World->Arena, component_render_mesh);
	}
	if( EntityFlags & COMPONENT_TYPE_LIGHT )
	{
		Entity->LightComponent = (component_light*) PushStruct(&World->Arena, component_light);
	}
	if( EntityFlags & COMPONENT_TYPE_COLLIDABLE_RECT )
	{
		Entity->CollidableRectComponent = (component_collidable_rect*) PushStruct(&World->Arena, component_collidable_rect);
	}
}

entity* NewEntity( world* World )
{
	Assert( World )
	Assert(  World->NrEntities < World->NrMaxEntities);

	entity* NewEntity = &World->Entities[World->NrEntities++];

	NewEntity->id 	 = EntityID++;

	return NewEntity;
}


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