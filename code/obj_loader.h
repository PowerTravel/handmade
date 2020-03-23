#pragma once

#include "math/vector_math.h"
#include "math/aabb.h"
#include "component_mesh.h"
#include "platform.h"

struct bitmap;
struct world;
struct game_state;
struct thread_context;
struct entity;

struct mtl_material
{
  u32 NameLength;
  char* Name;

  v4 *Kd;
  v4 *Ka;
  v4 *Tf;
  v4 *Ks;
  r32 *Ni;
  r32 *Ns;

  r32 BumpMapBM;
  bitmap* BumpMap;
  bitmap* MapKd;
  bitmap* MapKs;
};

struct obj_mtl_data
{
  u32 MaterialCount;
  mtl_material* Materials;
};

struct obj_group
{
  u32 GroupNameLength;
  char* GroupName;

  mesh_indeces Indeces;

  aabb3f aabb;

  mtl_material* Material;
};

struct obj_loaded_file
{
  u32 ObjectCount;
  obj_group* Objects;

  mesh_data* MeshData;

  obj_mtl_data* MaterialData;
};

obj_loaded_file* ReadOBJFile(thread_context* Thread, game_state* aGameState,
         debug_platform_read_entire_file* ReadEntireFile,
         debug_platfrom_free_file_memory* FreeEntireFile,
         char* FileName);

bitmap* LoadTGA( thread_context* Thread, memory_arena* AssetArena,
         debug_platform_read_entire_file* ReadEntireFile,
         debug_platfrom_free_file_memory* FreeEntireFile,
         char* FileName);
entity* CreateEntityFromOBJGroup( world* World, obj_group* OBJGrp, mesh_data* MeshData );
void CreateEntitiesFromOBJFile( world* World, obj_loaded_file* ObjFile );
