#include "epa_collision_data.h"
#include "handmade.h"
#include "handmade_tile.h"
#include "utility_macros.h"
#include "entity_components.h"
#include "component_camera.h"
#include "component_controller.h"
#include "component_light.h"
#include "component_mesh.h"
#include "component_spatial.h"
#include "component_collider.h"
#include "component_dynamics.h"
#include "component_surface.h"
#include "component_sprite_animation.h"

void NewComponents( world* World, entity* Entity, u32 EntityFlags )
{
	Assert( EntityFlags );
	Assert( ! ( EntityFlags & ( ~(COMPONENT_TYPE_FINAL - 1) ) ) );

	Entity->Types = Entity->Types | EntityFlags;

	if( EntityFlags & COMPONENT_TYPE_CAMERA )
	{
		Entity->CameraComponent = (component_camera*) PushStruct(World->PersistentArena, component_camera);
	}
	if( EntityFlags & COMPONENT_TYPE_LIGHT )
	{
		Entity->LightComponent = (component_light*) PushStruct(World->PersistentArena, component_light);
	}
	if( EntityFlags & COMPONENT_TYPE_CONTROLLER )
	{
		Entity->ControllerComponent = (component_controller*) PushStruct(World->PersistentArena, component_controller);
	}
	if( EntityFlags & COMPONENT_TYPE_MESH )
	{
		Entity->MeshComponent = (component_mesh*) PushStruct(World->PersistentArena, component_mesh);
	}
	if( EntityFlags & COMPONENT_TYPE_SPATIAL )
	{
		Entity->SpatialComponent = (component_spatial*) PushStruct(World->PersistentArena, component_spatial);
	}
	if( EntityFlags & COMPONENT_TYPE_COLLIDER )
	{
		Assert(Entity->SpatialComponent); // Collision Requires Spatial
		Entity->ColliderComponent = (component_collider*) PushStruct(World->PersistentArena, component_collider);
	}
	if( EntityFlags & COMPONENT_TYPE_DYNAMICS )
	{
		Assert(Entity->SpatialComponent && Entity->ColliderComponent); // Dynamics Requires Spatial and Collision
		Entity->DynamicsComponent = (component_dynamics*) PushStruct(World->PersistentArena, component_dynamics);
	}
	if( EntityFlags & COMPONENT_TYPE_SURFACE )
	{
		Entity->SurfaceComponent = (component_surface*) PushStruct(World->PersistentArena, component_surface);
	}
	if( EntityFlags & COMPONENT_TYPE_SPRITE_ANIMATION )
	{
		Entity->SpriteAnimationComponent = (component_sprite_animation*) PushStruct(World->PersistentArena, component_sprite_animation);
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
