
#include "entity_components.h"

#include "utility_macros.h"

world* AllocateWorld( u32 NrMaxEntities )
{
	world* Result = BootstrapPushStruct( world, Arena );

	InitializeTileMap( &Result->TileMap );

	Result->NrEntities = 0;
	Result->NrMaxEntities = NrMaxEntities;
	Result->Entities = (entity*) PushArray( &Result->Arena, NrMaxEntities, entity );

	return Result;
}

void NewComponents( world* World, entity* Entity, u32 EntityFlags )
{
	Assert( EntityFlags );
	Assert( ! ( EntityFlags & ( ~(COMPONENT_TYPE_FINAL - 1) ) ) );

	Entity->Types = Entity->Types | EntityFlags;

	if( EntityFlags & COMPONENT_TYPE_CAMERA )
	{
		Entity->CameraComponent = (component_camera*) PushStruct(&World->Arena, component_camera);
	}
	if( EntityFlags & COMPONENT_TYPE_LIGHT )
	{
		Entity->LightComponent = (component_light*) PushStruct(&World->Arena, component_light);
	}
	if( EntityFlags & COMPONENT_TYPE_CONTROLLER )
	{
		Entity->ControllerComponent = (component_controller*) PushStruct(&World->Arena, component_controller);
	}
	if( EntityFlags & COMPONENT_TYPE_MESH )
	{
		Entity->MeshComponent = (component_mesh*) PushStruct(&World->Arena, component_mesh);
	}
	if( EntityFlags & COMPONENT_TYPE_SPATIAL )
	{
		Entity->SpatialComponent = (component_spatial*) PushStruct(&World->Arena, component_spatial);
	}
	if( EntityFlags & COMPONENT_TYPE_SURFACE )
	{
		Entity->SurfaceComponent = (component_surface*) PushStruct(&World->Arena, component_surface);
	}
	if( EntityFlags & COMPONENT_TYPE_SPRITE_ANIMATION )
	{
		Entity->SpriteAnimationComponent = (component_sprite_animation*) PushStruct(&World->Arena, component_sprite_animation);
	}
}

entity* NewEntity( world* World )
{
	Assert( World );
	Assert(  World->NrEntities < World->NrMaxEntities);

	entity* NewEntity = &World->Entities[World->NrEntities++];

	NewEntity->id 	 = EntityID++;

	return NewEntity;
}

