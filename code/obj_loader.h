#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

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

  v3 CenterOfMass;
  v3 BoundingBoxMin;
  v3 BoundingBoxMax;

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

void CreateEntitiesFromOBJFile( world* World, obj_loaded_file* ObjFile );

#endif OBJ_LOADER_H