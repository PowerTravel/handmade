#ifndef ENTITY_COMPONENTS_H
#define ENTITY_COMPONENTS_H

static s32 EntityID = 0;

struct triangle
{
	s32 vi[3];
	s32 vni[3];
	b32 HasVerticeNormals;
	v4 n;
};

struct face
{
	s32  nv;	// Nr Vertices
	s32* vi; 	// Vertex Indeces
	s32* ni;	// Normal Indeces
	s32* ti;  	// Texture Indeces
};

struct obj_geometry
{
	s32 nv;		 // Nr Verices
	v4* v;		 // Vertices

	s32 nvn; 	 // Nr Vertice Normals
	v4* vn;		 // Vertice Normals

	s32 nvt; 	 // Nr Trxture Vertices 
	v3* vt; 	 // Texture Vertices

	s32   nf;	 // Nr Faces
	face* f; 	 // Faces

	s32 nt; 	 // Nr Triangles
	triangle* t; // Triangles
};


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

struct component_material
{
	v4 AmbientColor;
	v4 DiffuseColor;
	v4 SpecularColor;
	r32 Shininess;
};

struct component_mesh
{
	m4 T;
	obj_geometry* Object;
};

struct component_position
{
	v3 Position;
	v3 Velocity;
	v3 Acceleration;
};

struct loaded_bitmap
{
	s32   Width;
	s32   Height;
	void* Pixels;
};

struct component_texture
{
	loaded_bitmap Bitmap;
};

struct entity
{
	u32 id;
	u32 Types;

	component_camera*		CameraComponent;
	component_controller*  	ControllerComponent;
	component_light*		LightComponent;
	component_material* 	MaterialComponent;
	component_mesh* 		MeshComponent;
	component_position*    	PositionComponent;
	component_texture*    	TextureComponent;

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
	COMPONENT_TYPE_EMPTY        = 0x00,
	COMPONENT_TYPE_CAMERA 		= 0x01,
	COMPONENT_TYPE_LIGHT   		= 0x02,
	COMPONENT_TYPE_CONTROLLER   = 0x04,
	COMPONENT_TYPE_MATERIAL 	= 0x08,
	COMPONENT_TYPE_MESH 	  	= 0x10,
	COMPONENT_TYPE_POSITION 	= 0x20,
	COMPONENT_TYPE_TEXTURE 		= 0x40,
	COMPONENT_TYPE_FINAL 		= 0x80
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

entity* AllocateNewEntity( world* World, u32 EntityFlags )
{
	Assert( EntityFlags )
	Assert( ! ( EntityFlags & ( ~(COMPONENT_TYPE_FINAL - 1) ) ) )
	Assert( World )
	Assert(  World->NrEntities < World->NrMaxEntities);

	entity* NewEntity = &World->Entities[World->NrEntities++];

	NewEntity->id 	 = EntityID++;
	NewEntity->Types = EntityFlags;

	if( EntityFlags & COMPONENT_TYPE_CAMERA )
	{
		NewEntity->CameraComponent = (component_camera*) PushStruct(&World->Arena, component_camera);
	}
	if( EntityFlags & COMPONENT_TYPE_CONTROLLER )
	{
		NewEntity->ControllerComponent = (component_controller*) PushStruct(&World->Arena, component_controller);
	}
	if( EntityFlags & COMPONENT_TYPE_MATERIAL )
	{
		NewEntity->MaterialComponent = (component_material*) PushStruct(&World->Arena, component_material);
	}
	if( EntityFlags & COMPONENT_TYPE_MESH )
	{
		NewEntity->MeshComponent = (component_mesh*) PushStruct(&World->Arena, component_mesh);
	}
	if( EntityFlags & COMPONENT_TYPE_LIGHT )
	{
		NewEntity->LightComponent = (component_light*) PushStruct(&World->Arena, component_light);
	}
	if( EntityFlags & COMPONENT_TYPE_POSITION )
	{
		NewEntity->PositionComponent = (component_position*) PushStruct(&World->Arena, component_position);
	}
	if( EntityFlags & COMPONENT_TYPE_TEXTURE )
	{
		NewEntity->TextureComponent = (component_texture*) PushStruct(&World->Arena, component_texture);
	}

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