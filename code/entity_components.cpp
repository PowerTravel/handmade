#include "epa_collision_data.h"
#include "handmade.h"
#include "handmade_tile.h"
#include "utility_macros.h"
#include "entity_components.h"
#include "component_camera.h"
#include "component_sprite_animation.h"

void NewComponents(world* World, entity* Entity, u32 EntityFlags )
{
	Assert( EntityFlags );
	Assert( ! ( EntityFlags & ( ~(COMPONENT_TYPE_FINAL - 1) ) ) );

	Entity->Types = Entity->Types | EntityFlags;

	if( EntityFlags & COMPONENT_TYPE_CAMERA )
	{
		Entity->CameraComponent = PushStruct(World->Arena, component_camera);
	}
	if( EntityFlags & COMPONENT_TYPE_LIGHT )
	{
		Entity->LightComponent = PushStruct(World->Arena, component_light);
	}
	if( EntityFlags & COMPONENT_TYPE_CONTROLLER )
	{
		Entity->ControllerComponent = PushStruct(World->Arena, component_controller);
	}
	if( EntityFlags & COMPONENT_TYPE_SPATIAL )
	{
		Entity->SpatialComponent =  PushStruct(World->Arena, component_spatial);
	}
	if( EntityFlags & COMPONENT_TYPE_COLLIDER )
	{
		Assert(Entity->SpatialComponent); // Collision Requires Spatial
		Entity->ColliderComponent = PushStruct(World->Arena, component_collider);
	}
	if( EntityFlags & COMPONENT_TYPE_DYNAMICS )
	{
		Assert(Entity->SpatialComponent && Entity->ColliderComponent); // Dynamics Requires Spatial and Collision
		Entity->DynamicsComponent = PushStruct(World->Arena, component_dynamics);
	}
	if( EntityFlags & COMPONENT_TYPE_SPRITE_ANIMATION )
	{
		Entity->SpriteAnimationComponent = PushStruct(World->Arena, component_sprite_animation);
	}
	if( EntityFlags & COMPONENT_TYPE_RENDER )
	{
		Entity->RenderComponent = PushStruct(World->Arena, component_render);
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

entity* CreateCameraEntity(world* World, r32 AngleOfView, r32 AspectRatio )
{
	entity* CameraEntity = NewEntity( World );
	NewComponents( World, CameraEntity, COMPONENT_TYPE_CAMERA );
	SetCameraComponent(CameraEntity->CameraComponent, AngleOfView, AspectRatio );
	return CameraEntity;
}
