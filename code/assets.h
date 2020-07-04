#pragma once

#include "bitmap.h"
#include "platform.h"
#include "memory.h"
#include "utility_macros.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "externals/stb_truetype.h"

struct buffer_location
{
  u32 Count;
  u8* Index;
  u32 VertexOffset;
};

struct book_keeper
{
  opengl_handles BufferHandle;
  buffer_location Location;
  b32 Loaded;
  u32 ReferenceCount;
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
  u32 MeshHandle;
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

material CreateMaterial( v4 AmbientColor, v4 DiffuseColor, v4 SpecularColor, r32 Shininess, b32 Emissive, u32 TextureHandle)
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

  u32 BitmapIndex;
  stbtt_bakedchar* CharData;
};


// TODO: Get rid of asset_instance/
// Let whatever thing that wants a reference
// to an asset keep track of it themselves
struct asset_instance
{

  // TODO: have a struct for each type of handle
  // object_handle{ Value }
  // material_handle{ Value }
  // texture_handle{ Value }
  // to enforce typesaftey. (meybe we can even remove the need for asset_type and use the handle type as a proxy)
  u32 ObjectIndex;
  u32 MaterialIndex;
  u32 TextureIndex;
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

  asset_vector   Instances;
  u32 TemporaryInstancesBase;
  u32 TemporaryInstancesCount;

  asset_vector   Meshes;
  asset_hash_map Bitmaps;
  asset_hash_map Objects;
  asset_hash_map Materials;

  book_keeper* ObjectKeeper;
  book_keeper* BitmapKeeper;

  u32 ObjectPendingLoadCount;
  u32 ObjectPendingLoad[256];

  u32 BitmapPendingLoadCount;
  u32 BitmapPendingLoad[256];

  stb_font_map FontMap;

  u32* EnumeratedMeshes;
};

// Game Layer API
u32 GetAssetHandle(game_asset_manager* AssetManager);
void SetAsset(game_asset_manager* AssetManager, asset_type AssetType, char* Name, u32 Handle);
u32 GetTemporaryAssetHandle(game_asset_manager* AssetManager);

collider_mesh GetColliderMesh( game_asset_manager* AssetManager, u32 Handle);
aabb3f GetMeshAABB(game_asset_manager* AssetManager, u32 Handle);

u32 GetAssetIndex(game_asset_manager* AssetManager, asset_type AssetType, char* Key);

void ResetAssetManagerTemporaryInstances(game_asset_manager* AssetManager);

void CopyAssets(game_asset_manager* AssetManager, u32 SrcHandle, u32 DstHandle);

// GL Layer API
inline u32 GetEnumeratedObjectIndex(game_asset_manager* AssetManager, predefined_mesh MeshType);

mesh_indeces* GetObject(game_asset_manager* AssetManager, u32 Handle, book_keeper** Keeper = NULL );
bitmap* GetBitmap(game_asset_manager* AssetManager, u32 Handle, book_keeper** Keeper = NULL );
mesh_data* GetMesh(game_asset_manager* AssetManager, u32 Handle);
material* GetMaterial(game_asset_manager* AssetManager, u32 Handle);

inline mesh_indeces* GetObjectFromIndex(game_asset_manager* AssetManager, u32 Index, book_keeper** Keeper = NULL );
inline bitmap* GetBitmapFromIndex(game_asset_manager* AssetManager, u32 Index, book_keeper** Keeper = NULL );
inline mesh_data* GetMeshFromIndex(game_asset_manager* AssetManager, u32 Index);
inline material* GetMaterialFromIndex(game_asset_manager* AssetManager, u32 Index );






