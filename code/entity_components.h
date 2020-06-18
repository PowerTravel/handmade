#pragma once

#include "types.h"

#include "math/affine_transformations.h"

static s32 EntityID = 0;

struct component_camera;
struct component_sprite_animation;
struct world;

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

// Requires spatial and collision primitive
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

//struct component_vector{
//  u32 Type;
//  void** Component;
//};

struct entity
{
  u32 id;
  u32 Types;

  component_camera*           CameraComponent;
  component_controller*       ControllerComponent;
  component_light*            LightComponent;
  component_spatial*          SpatialComponent;
  component_collider*         ColliderComponent;
  component_dynamics*         DynamicsComponent;
  component_sprite_animation* SpriteAnimationComponent;
  component_render*           RenderComponent;
};

enum component_types
{
  COMPONENT_TYPE_EMPTY              = 0x0000,
  COMPONENT_TYPE_CAMERA             = 0x0001,
  COMPONENT_TYPE_LIGHT              = 0x0002,
  COMPONENT_TYPE_CONTROLLER         = 0x0004,
  COMPONENT_TYPE_SPATIAL            = 0x0008,
  COMPONENT_TYPE_COLLIDER           = 0x0010,
  COMPONENT_TYPE_DYNAMICS           = 0x0020,
  COMPONENT_TYPE_SPRITE_ANIMATION   = 0x0040,
  COMPONENT_TYPE_RENDER             = 0x0080,
  COMPONENT_TYPE_FINAL              = 0x0100
};

void NewComponents( world* World, entity* Entity, u32 EntityFlags );
entity* NewEntity( world* World );
entity* CreateCameraEntity(world* World, r32 AngleOfView, r32 AspectRatio );