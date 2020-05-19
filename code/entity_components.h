#pragma once

#include "types.h"

static s32 EntityID = 0;

struct component_camera;
struct component_controller;
struct component_light;
struct component_spatial;
struct component_collider;
struct component_dynamics;
struct component_sprite_animation;
struct world;
struct asset_handle;

struct component_render
{
  u32 MeshHandle;
  u32 TextureHandle;
};

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
//  COMPONENT_TYPE_MESH               = 0x0008,
  COMPONENT_TYPE_SPATIAL            = 0x0010,
  COMPONENT_TYPE_COLLIDER           = 0x0020,
  COMPONENT_TYPE_DYNAMICS           = 0x0040,
//  COMPONENT_TYPE_SURFACE            = 0x0080,
  COMPONENT_TYPE_SPRITE_ANIMATION   = 0x0100,
  COMPONENT_TYPE_RENDER             = 0x0200,
  COMPONENT_TYPE_FINAL              = 0x0400
};


void NewComponents( world* World, entity* Entity, u32 EntityFlags );
entity* NewEntity( world* World );
entity* CreateCameraEntity(world* World, r32 AngleOfView, r32 AspectRatio );