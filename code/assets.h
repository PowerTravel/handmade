#pragma once

#include "bitmap.h"
#include "platform.h"
#include "memory.h"
#include "utility_macros.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "externals/stb_truetype.h"

struct object_handle
{
  u32 Value;
};

struct mesh_handle
{
  u32 Value;
};

struct material_handle
{
  u32 Value;
};

struct bitmap_handle
{
  u32 Value;
};

struct bitmap_keeper
{
  b32 Loaded;
  u32 TextureHandle;

  u32 TextureSlot;
  b32 Special;
  b32 Referenced;

  b32 UseSubRegion;
  rect2f SubRegion;
};

struct buffer_keeper
{
  b32 Loaded;
  u32 VAO;  // Vertex Array Object
  u32 VBO;  // Vertex Buffer Object
  u32 EBO;  // Element Buffer Object

  u32 Count;
  u8* Index;
  u32 VertexOffset;
  b32 Referenced;
};

struct mesh_data
{
  u32 nv;    // Nr Verices
  u32 nvn;   // Nr Vertice Normals
  u32 nvt;   // Nr Trxture Vertices

  v3* v;     // Vertices
  v3* vn;    // Vertice Normals
  v2* vt;    // Texture Vertices
};

struct collider_mesh
{
  u32 nv;   // Nr Vertices
  u32 nvi;  // 3 times nr Vertice Indeces (CCW Triangles)

  v3* v;    // Vertices
  u32* vi;  // Vertex Indeces
};

struct mesh_indeces
{
  mesh_handle MeshHandle;
  u32 Count;  // 3 times Nr Triangles
  u32* vi;    // Vertex Indeces
  u32* ti;    // Texture Indeces
  u32* ni;    // Normal Indeces
  aabb3f AABB;
  char Name[128];
};

struct material
{
  v4 AmbientColor;
  v4 DiffuseColor;
  v4 SpecularColor;
  r32 Shininess;
  b32 Emissive;
};

material CreateMaterial( v4 AmbientColor, v4 DiffuseColor, v4 SpecularColor, r32 Shininess, b32 Emissive)
{
  material Result = {};
  Result.AmbientColor = AmbientColor;
  Result.DiffuseColor = DiffuseColor;
  Result.SpecularColor = SpecularColor;
  Result.Shininess = Shininess;
  Result.Emissive = Emissive;
  return Result;
}

struct stb_font_map
{
  s32 StartChar;
  s32 NumChars;
  r32 FontHeightPx;

  r32 Ascent;
  r32 Descent;
  r32 LineGap;

  bitmap_handle BitmapHandle;
  stbtt_bakedchar* CharData;
};

enum class asset_type
{
  OBJECT,
  MATERIAL,
  BITMAP,
  INSTANCE,
  MESH
};

enum class predefined_mesh
{
  QUAD,
  VOXEL,
  COUNT
  // SPHERE, CYLINDER ....
};

struct asset_vector
{
  u32 Count;
  u32 MaxCount;
  void** Values;
};

struct asset_hash_map
{
  u32 Count;
  u32 MaxCount;
  c8* Keys[64];
  void** Values;
};

struct pending_asset
{
  u32 Handle;
  asset_type Type;
};

struct game_asset_manager
{
  memory_arena AssetArena;

  asset_vector   Meshes;
  asset_hash_map Bitmaps;
  asset_hash_map Objects;
  asset_hash_map Materials;

  buffer_keeper* ObjectKeeper;
  bitmap_keeper* BitmapKeeper;

  u32 ObjectPendingLoadCount;
  object_handle ObjectPendingLoad[64];

  u32 BitmapPendingLoadCount;
  bitmap_handle BitmapPendingLoad[64];

  stb_font_map FontMap[10];

  object_handle* EnumeratedMeshes;
};

collider_mesh GetColliderMesh( game_asset_manager* AssetManager, object_handle Handle);
aabb3f GetMeshAABB( game_asset_manager* AssetManager, object_handle Handle);

void ResetAssetManagerTemporaryInstances(game_asset_manager* AssetManager);

// GL Layer API
inline object_handle GetEnumeratedObjectHandle(game_asset_manager* AssetManager, predefined_mesh MeshType);

void GetHandle(game_asset_manager* AssetManager, char* Key, bitmap_handle* Handle);
void GetHandle(game_asset_manager* AssetManager, char* Key, object_handle* Handle);
void GetHandle(game_asset_manager* AssetManager, char* Key, material_handle* Handle);

inline mesh_indeces* GetAsset(game_asset_manager* AssetManager, object_handle Handle, buffer_keeper** Keeper = NULL);
inline mesh_data* GetAsset(game_asset_manager* AssetManager, mesh_handle Index);//, buffer_keeper** Keeper = NULL);
inline bitmap* GetAsset(game_asset_manager* AssetManager, bitmap_handle Handle, bitmap_keeper** Keeper = NULL);
inline material* GetAsset(game_asset_manager* AssetManager, material_handle Handle, u32 Index );


stb_font_map* GetFontMap(game_asset_manager* AssetManager, u32 FontSize)
{
  Assert(ArrayCount(AssetManager->FontMap) == 10);
  stb_font_map* Result = 0;
  for (int i = 0; i < ArrayCount(AssetManager->FontMap); ++i)
  {
    stb_font_map* Map = &AssetManager->FontMap[i];
    if(Map->FontHeightPx == FontSize)
    {
      Result = Map;
      break;
    }

  }
  Assert(Result);
  return Result;
}




