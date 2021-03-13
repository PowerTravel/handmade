#pragma once

#include "math/aabb.h"
#include "containers/vector_list.h"
struct entity;
struct memory_arena;

struct aabb_tree_node
{
  aabb3f AABB;
  r32 Border;
  u32 EntityID;
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
  u32 EntityIDA;
  u32 EntityIDB;
  broad_phase_result_stack* Previous;
};

u32 GetAABBList(aabb_tree* Tree, aabb3f** Result);
broad_phase_result_stack* GetCollisionPairs( aabb_tree* Tree,  u32* ResultStackSize);
void AABBTreeInsert( memory_arena* Arena, aabb_tree* Tree, u32 EntityID, aabb3f& AABBWorldSpace );
void PointPick( memory_arena* Arena, aabb_tree* Tree, v3* point, v3 direction, vector_list<u32> & Result);
aabb_tree BuildBroadPhaseTree( );