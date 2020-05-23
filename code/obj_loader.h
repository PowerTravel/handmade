#pragma once

#include "math/vector_math.h"
#include "math/aabb.h"
#include "platform.h"

struct bitmap;

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

struct mesh_indeces;
struct obj_group
{
  u32 GroupNameLength;
  char* GroupName;

  mesh_indeces* Indeces;

  aabb3f aabb;

  mtl_material* Material;
};

struct mesh_data;
struct obj_loaded_file
{
  u32 ObjectCount;
  obj_group* Objects;

  mesh_data* MeshData;

  obj_mtl_data* MaterialData;
};

obj_loaded_file* ReadOBJFile(memory_arena* AssetArena, memory_arena* TempArena, char* FileName);
bitmap* LoadTGA(memory_arena* AssetArena, char* FileName);
