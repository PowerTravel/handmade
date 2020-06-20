#pragma once

#include "types.h"

#include "math/affine_transformations.h"

#include "bitmap.h"
#include "data_containers.h"

struct component_base;
struct entity_component_dlist
{
  component_base* ComponentBase;
  entity_component_dlist* Previous;
  entity_component_dlist* Next;
};
struct entity
{
  u32 ID;
  u32 ComponentFlags;
  entity_component_dlist* ComponentSentinel;
};

struct component_base
{
  entity* Entity;
  u32 TypeFlag;
};

struct component_camera
{
  r32 AngleOfView;
  r32 AspectRatio;
  m4  DeltaRot;
  v3  DeltaPos;
  m4  V;
  m4  P;
};

struct game_controller_input;
enum ControllerType
{
  ControllerType_Hero,
  ControllerType_FlyingCamera,
};
struct component_controller
{
  game_controller_input* Controller;
  u32 Type;
};

struct component_light
{
  v4 Color;
};

struct component_spatial
{
  component_spatial(v3 PosInit = V3(0,0,0), v3 ScaleInit = V3(1,1,1), v4 RotInit = V4(0,0,0,1) ) :
        Position(PosInit), Scale(ScaleInit), Rotation(RotInit){};
  v3 Scale;
  v3 Position;
  //  we define the Quaternion as (xi,yj,zk, Scalar)
  //  Some resources define it as (Scalar,xi,yj,zk)
  v4 Rotation;
};

// TODO: Optimize by doing explicit element multiplication
m4 GetModelMatrix( const component_spatial* c )
{
  //TIMED_FUNCTION();
  const m4 Result = M4Identity();
  const m4 Scale = GetScaleMatrix(V4(c->Scale,1));
  const m4 Rotation = GetRotationMatrix(c->Rotation);
  const m4 Translation = GetTranslationMatrix(c->Position);
  return Translation * Rotation * Scale;
}

struct component_collider
{
  // Always in Model Space
  aabb3f AABB;
  u32 AssetHandle;
};

struct component_dynamics
{
  v3  LinearVelocity;
  v3  AngularVelocity;
  v3  ExternalForce;
  r32 Mass;
};

struct component_render
{
  u32 AssetHandle;
};

// TODO: Move to AssetManager.
// A sprite animation is just a static lookup table setup at start.
// Perfect for AssetManager to deal with.
// This component can still exist but only keeps a handle to the active subframe.
struct component_sprite_animation
{
  hash_map< list <m4> > Animation;
  list<m4>* ActiveSeries;
  b32 InvertX;
};

enum component_type
{
  COMPONENT_FLAG_NONE               = 0x0000,
  COMPONENT_FLAG_CAMERA             = 0x0001,
  COMPONENT_FLAG_LIGHT              = 0x0002,
  COMPONENT_FLAG_CONTROLLER         = 0x0004,
  COMPONENT_FLAG_SPATIAL            = 0x0008,
  COMPONENT_FLAG_COLLIDER           = 0x0010,
  COMPONENT_FLAG_DYNAMICS           = 0x0020,
  COMPONENT_FLAG_RENDER             = 0x0040,
  COMPONENT_FLAG_SPRITE_ANIMATION   = 0x0080,
  COMPONENT_FLAG_FINAL              = 0x0100,
};


enum component_index
{
  COMPONENT_INDEX_CAMERA,
  COMPONENT_INDEX_LIGHT,
  COMPONENT_INDEX_CONTROLLER,
  COMPONENT_INDEX_SPATIAL,
  COMPONENT_INDEX_COLLIDER,
  COMPONENT_INDEX_DYNAMICS,
  COMPONENT_INDEX_RENDER,
  COMPONENT_INDEX_SPRITE_ANIMATION,
  COMPONENT_MAX_INDEX
};

struct em_chunk
{
  midx Used;
  u8* Memory;
  em_chunk* Previous;
};

struct component_list
{
  u32 ComponentTypeFlag;
  u32 ComponentRequirement;
  midx ComponentSize;
  midx ChunkSize;
  em_chunk* Last;
};

component_list ComponentList(u32 Flag, midx ComponentSize, midx ChunkComponentCount, u32 Requires)
{
  component_list Result = {};
  Result.ComponentTypeFlag = Flag;
  Result.ComponentRequirement = Requires;
  Result.ComponentSize = sizeof(component_base) + ComponentSize;
  Result.ChunkSize = ChunkComponentCount * Result.ComponentSize;
  return Result;
}

struct entity_manager
{
  memory_arena Arena;

  u32 EntityCount;
  u32 EntitiesPerChunk;
  u32 EntitycChunkCount;
  em_chunk* EntityList;

  u32 ComponentCount;
  component_list* Components;
};

entity_manager* CreateEntityManager( )
{
  entity_manager* Result = BootstrapPushStruct(entity_manager, Arena);

  u32 CameraChunkCount = 10;
  u32 LightChunkCount = 10;
  u32 ControllerChunkCount = 4;
  u32 EntityChunkCount = 128;

  Result->ComponentCount = COMPONENT_MAX_INDEX;
  Result->Components = PushArray( &Result->Arena, COMPONENT_MAX_INDEX, component_list);
  Result->Components[COMPONENT_INDEX_CAMERA] = ComponentList(COMPONENT_FLAG_CAMERA, sizeof(component_camera), CameraChunkCount, COMPONENT_FLAG_NONE);
  Result->Components[COMPONENT_INDEX_LIGHT] = ComponentList(COMPONENT_FLAG_LIGHT, sizeof(component_light), LightChunkCount, COMPONENT_FLAG_SPATIAL),
  Result->Components[COMPONENT_INDEX_CONTROLLER] = ComponentList(COMPONENT_FLAG_CONTROLLER, sizeof(component_controller), ControllerChunkCount, COMPONENT_FLAG_NONE);
  Result->Components[COMPONENT_INDEX_SPATIAL] = ComponentList(COMPONENT_FLAG_SPATIAL, sizeof(component_spatial), EntityChunkCount, COMPONENT_FLAG_NONE);
  Result->Components[COMPONENT_INDEX_COLLIDER] = ComponentList(COMPONENT_FLAG_COLLIDER, sizeof(component_collider), EntityChunkCount, COMPONENT_FLAG_SPATIAL);
  Result->Components[COMPONENT_INDEX_DYNAMICS] = ComponentList(COMPONENT_FLAG_DYNAMICS, sizeof(component_dynamics), EntityChunkCount, COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_COLLIDER);
  Result->Components[COMPONENT_INDEX_RENDER] = ComponentList(COMPONENT_FLAG_RENDER, sizeof(component_render), EntityChunkCount, COMPONENT_FLAG_NONE);
  Result->Components[COMPONENT_INDEX_SPRITE_ANIMATION] = ComponentList(COMPONENT_FLAG_SPRITE_ANIMATION, sizeof(component_sprite_animation), EntityChunkCount, COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_RENDER);

  Result->EntityCount = 0;
  Result->EntitycChunkCount = 1;
  Result->EntitiesPerChunk = EntityChunkCount;
  Result->EntityList = PushStruct( &Result->Arena, em_chunk);
  Result->EntityList->Used = 0;
  Result->EntityList->Memory = PushArray(&Result->Arena, Result->EntitiesPerChunk * sizeof(entity), u8);
  Result->EntityList->Previous = 0;

  return Result;
}

void NewComponents( entity_manager* EM, entity* Entity, u32 EntityFlags );
u32 NewEntity( entity_manager* EM );

struct component_result
{

};

component_result* GetComponentsOfType(entity_manager* EM, u32 ComponentFlags = 0)
{
  return 0;
};

bool Next(entity_manager* EM, component_result* ComponentList)
{
  return false;
}

entity* GetEntity( u8* Component )
{
  component_base* Base = (component_base*) (Component - sizeof(component_base));
  return Base->Entity;
}

u8* GetComponent(entity_manager* EM, component_result* ComponentList, u32 ComponentFlag)
{
  return 0;
}

u8* GetComponent(entity_manager* EM, u32 EntityID, u32 ComponentFlag);


#define GetCameraComponent(Input) ((component_camera*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_CAMERA))
#define GetLightComponent(Input) ((component_light*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_LIGHT))
#define GetControllerComponent(Input) ((component_controller*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_CONTROLLER))
#define GetSpatialComponent(Input) ((component_spatial*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_SPATIAL))
#define GetColliderComponent(Input) ((component_collider*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_COLLIDER))
#define GetDynamicsComponent(Input) ((component_dynamics*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_DYNAMICS))
#define GetRenderComponent(Input) ((component_render*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_RENDER))
#define GetSpriteAnimationComponent(Input) ((component_sprite_animation*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_SPRITE_ANIMATION))