#pragma once

#include "types.h"

#include "math/affine_transformations.h"

#include "bitmap.h"
#include "data_containers.h"
#include "Assets.h"

struct component_head;

struct entity_component_chunk
{
  u32 Types;
  component_head** Components;
  
  entity_component_chunk* Next;
};

struct entity
{
  u32 ID; // ID starts at 1. Index is ID-1
  u32 ComponentFlags;
  entity_component_chunk* Components;
};

struct component_head
{
  entity* Entity;
  u32 Type;
};


// Todo, Can we use a location component instead of DeltaRot, DeltaPos etc?
struct component_camera
{
  r32 AngleOfView;
  r32 AspectRatio;
  m4  DeltaRot;
  v3  DeltaPos;
  m4  V;
  m4  P;
  b32 Active;
};

struct game_controller_input;
enum ControllerType
{
  ControllerType_Hero,
  ControllerType_FlyingCamera,
};
struct component_controller
{
  keyboard_input* Keyboard;
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
  m4 ModelMatrix;
};

void UpdateModelMatrix( component_spatial* c )
{
  TIMED_FUNCTION();
  const m4 Scale = GetScaleMatrix(V4(c->Scale,1));
  const m4 Rotation = GetRotationMatrix(c->Rotation);
  const m4 Translation = GetTranslationMatrix(c->Position);
  c->ModelMatrix = Translation * Rotation * Scale;
}

struct component_collider
{
  // Always in Model Space
  aabb3f AABB;
  object_handle Object;
};

struct component_dynamics
{
  v3  LinearVelocity;
  v3  AngularVelocity;
  v3  ExternalForce;
  r32 Mass;
  m3 I;     // Inertial tensor around CM aligned with principal axis
  m3 I_inv; // Inverse of I
};

struct component_render
{
  object_handle Object;
  material_handle Material;
  bitmap_handle Bitmap;
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

struct em_chunk
{
  midx Used;
  u8* Memory;
  em_chunk* Next;
};

struct component_list
{
  u32 Type;
  u32 Requirement;
  u32 Count;
  midx ComponentSize;
  midx ChunkSize;
  em_chunk* First;
};

component_list ComponentList(u32 Flag, midx ComponentSize, midx ChunkComponentCount, u32 Requires)
{
  component_list Result = {};
  Result.Type = Flag;
  Result.Requirement = Requires;
  Result.ComponentSize = sizeof(component_head) + ComponentSize;
  Result.ChunkSize = ChunkComponentCount * Result.ComponentSize;
  return Result;
}

struct entity_manager
{
  memory_arena Arena;
  temporary_memory TemporaryMemory;
  
  u32 EntityCount;
  u32 EntitiesPerChunk;
  u32 EntityChunkCount;
  em_chunk* EntityList;
  
  u32 ComponentCount;
  component_list* Components;
};

entity_manager* CreateEntityManager( );
void NewComponents( entity_manager* EM, entity* Entity, u32 EntityFlags );
u32 NewEntity( entity_manager* EM );

void BeginTransactions(entity_manager* EM)
{
  CheckArena(&EM->Arena);
  EM->TemporaryMemory = BeginTemporaryMemory(&EM->Arena);
}

void EndTransactions(entity_manager* EM)
{
  EndTemporaryMemory(EM->TemporaryMemory);
  CheckArena(&EM->Arena);
}

#define ScopedTransaction(EntityManager) scoped_temporary_memory ScopedMemory_ = scoped_temporary_memory(&(EntityManager)->Arena)

struct filtered_components
{
  u32 Count;
  component_head* Heads;
};

struct component_result
{
  entity_manager* EM;
  u32 MainType;
  midx MainTypeSize;
  u32 Types;
  b32 Begun;
  
  u32 ArrayCount;
  u32 ArrayIndex;
  u32 ComponentIndex;
  filtered_components* FilteredArray;
};

component_result* GetComponentsOfType(entity_manager* EM, u32 ComponentFlags);
b32 Next(entity_manager* EM, component_result* ComponentList);

u32 GetEntity( u8* Component )
{
  component_head* Base = (component_head*) (Component - sizeof(component_head));
  return Base->Entity->ID;
}

u8* GetComponent(entity_manager* EM, component_result* ComponentList, u32 ComponentFlag);
u8* GetComponent(entity_manager* EM, u32 EntityID, u32 ComponentFlag);


#define GetCameraComponent(Input) ((component_camera*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_CAMERA))
#define GetLightComponent(Input) ((component_light*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_LIGHT))
#define GetControllerComponent(Input) ((component_controller*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_CONTROLLER))
#define GetSpatialComponent(Input) ((component_spatial*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_SPATIAL))
#define GetColliderComponent(Input) ((component_collider*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_COLLIDER))
#define GetDynamicsComponent(Input) ((component_dynamics*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_DYNAMICS))
#define GetRenderComponent(Input) ((component_render*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_RENDER))
#define GetSpriteAnimationComponent(Input) ((component_sprite_animation*) GetComponent(GlobalGameState->EntityManager, Input, COMPONENT_FLAG_SPRITE_ANIMATION))