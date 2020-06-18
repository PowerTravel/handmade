#pragma once

#include "math/aabb.h"

struct entity;
struct memory_arena;

struct aabb_tree_node
{
  aabb3f AABB;
  r32 Border;
  entity* Entity;
  aabb_tree_node* Left;
  aabb_tree_node* Right;
  b32 Entered;
};

struct aabb_tree
{
  u32 Size;
  aabb_tree_node* Root;
};

struct broad_phase_result_stack
{
  entity* A;
  entity* B;
  broad_phase_result_stack* Previous;
};

u32 GetAABBList(aabb_tree* Tree, aabb3f** Result);
broad_phase_result_stack* GetCollisionPairs( aabb_tree* Tree,  u32* ResultStackSize);
void AABBTreeInsert( memory_arena* Arena, aabb_tree* Tree, entity* Entity , aabb3f& AABBWorldSpace );