#pragma once

#include "bitmap.h"
#include "platform.h"
#include "memory.h"
#include "utility_macros.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "externals/stb_truetype.h"

struct book_keeper
{
  opengl_handles BufferHandle;
  b32 Loaded;
  b32 Exists;
  b32 Dirty;
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

struct asset_instance
{
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

  stb_font_map FontMap;

  u32* EnumeratedMeshes;
};

// Game Layer API
u32 GetAssetHandle(game_asset_manager* AssetManager);
void SetAsset(game_asset_manager* AssetManager, asset_type AssetType, char* Name, u32 Handle);
u32 GetTemporaryAssetHandle(game_asset_manager* AssetManager);
void CopyAssets(game_asset_manager* AssetManager, u32 SrcHandle, u32 DstHandle);
u32 GetAssetIndex(game_asset_manager* AssetManager, asset_type AssetType, char* Key);
void ResetAssetManagerTemporaryInstances(game_asset_manager* AssetManager);


// GL Layer API
inline u32
GetEnumeratedObjectIndex(game_asset_manager* AssetManager, predefined_mesh MeshType)
{
  u32 Result = AssetManager->EnumeratedMeshes[(u32)MeshType];
  return Result;
}

internal asset_instance* GetAssetInstance(game_asset_manager* AssetManager, u32 Handle)
{
  Assert(Handle < AssetManager->Instances.Count + AssetManager->TemporaryInstancesCount);
  asset_instance* Result = (asset_instance*) AssetManager->Instances.Values[Handle];
  Assert(Result);
  return Result;
}

inline mesh_indeces* GetObjectFromIndex(game_asset_manager* AssetManager, u32 Index, book_keeper** Keeper = NULL )
{
  Assert(Index < AssetManager->Objects.MaxCount);
  mesh_indeces* Result = (mesh_indeces*) AssetManager->Objects.Values[Index];
  Assert(Result);

  if(Keeper) *Keeper = AssetManager->ObjectKeeper + Index;

  return Result;
}

mesh_indeces* GetObject(game_asset_manager* AssetManager, u32 Handle, book_keeper** Keeper = NULL )
{
  asset_instance* Instance = GetAssetInstance(AssetManager, Handle);
  mesh_indeces* Result = GetObjectFromIndex(AssetManager, Instance->ObjectIndex, Keeper);
  return Result;
};

inline mesh_data* GetMeshFromIndex(game_asset_manager* AssetManager, u32 Index)
{
  Assert(Index < AssetManager->Meshes.Count);
  mesh_data* Result = (mesh_data*) AssetManager->Meshes.Values[Index];
  Assert(Result);
  return Result;
}

mesh_data* GetMesh(game_asset_manager* AssetManager, u32 Handle)
{
  mesh_indeces* Object = GetObject(AssetManager, Handle );
  mesh_data* Result = GetMeshFromIndex(AssetManager, Object->MeshHandle);
  return Result;
}

collider_mesh GetColliderMesh( game_asset_manager* AssetManager, u32 Handle)
{
  mesh_indeces* Object = GetObject(AssetManager, Handle);
  mesh_data* Data = GetMesh(AssetManager, Handle);
  collider_mesh Result = {};
  Result.nv = Data->nv;
  Result.v = Data->v;
  Result.nvi = Object->Count;
  Result.vi = Object->vi;
  return Result;
}

aabb3f GetMeshAABB(game_asset_manager* AssetManager, u32 Handle)
{
  mesh_indeces* Object = GetObject(AssetManager, Handle);
  return Object->AABB;
}

inline material* GetMaterialFromIndex(game_asset_manager* AssetManager, u32 Index )
{
  Assert(Index < AssetManager->Materials.MaxCount);
  material* Result = (material*) AssetManager->Materials.Values[Index];
  Assert(Result);
  return Result;
}

material* GetMaterial(game_asset_manager* AssetManager, u32 Handle )
{
  asset_instance* Instance = GetAssetInstance(AssetManager, Handle);
  material* Result = GetMaterialFromIndex(AssetManager, Instance->MaterialIndex );
  return Result;
};

inline bitmap* GetBitmapFromIndex(game_asset_manager* AssetManager, u32 Index, book_keeper** Keeper = NULL )
{
  Assert(Index < AssetManager->Bitmaps.MaxCount);
  bitmap* Result = (bitmap*) AssetManager->Bitmaps.Values[Index];

  Assert(Result);

  if(Keeper) *Keeper = AssetManager->BitmapKeeper + Index;

  return Result;
}

bitmap* GetBitmap(game_asset_manager* AssetManager, u32 Handle, book_keeper** Keeper = NULL )
{
  asset_instance* Instance = GetAssetInstance(AssetManager, Handle);
  bitmap* Result = GetBitmapFromIndex(AssetManager, Instance->TextureIndex, Keeper);
  return Result;
}