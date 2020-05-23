#pragma once

#include "bitmap.h"
#include "platform.h"
#include "memory.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "externals/stb_truetype.h"

struct book_keeper
{
  // Todo: Is it okay to keep opengl specific data in the asset manager.
  //       I think so but im not sure.
  opengl_handles BufferHandle;
  b32 Loaded;
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
};

struct material
{
  v4 AmbientColor;
  v4 DiffuseColor;
  v4 SpecularColor;
  r32 Shininess;
  u32 TextureHandle;
  bitmap* DiffuseMap;
};

struct stb_font_map
{
  s32 StartChar;
  s32 NumChars;
  r32 FontHeightPx;

  r32 Ascent;
  r32 Descent;
  r32 LineGap;

  u32 BitmapHandle;
  stbtt_bakedchar* CharData;
};

struct game_asset_manager
{
  memory_arena AssetArena;

  u32 MaxObjectCount;
  u32 ObjectCount;
  mesh_indeces* Objects;

  u32 MaxMeshCount;
  u32 MeshCount;
  mesh_data* MeshData;

  u32 MaxTextureCount;
  u32 TextureCount;
  bitmap* Textures;

  u32 MaxMaterialCount;
  u32 MaterialCount;
  material* Materials;

  book_keeper* ObjectKeeper;
  book_keeper* TextureKeeper;

  stb_font_map FontMap;
};

struct collider_mesh;
struct component_render;

// Note: TextureIndex and MeshIndex are not the same as MeshHandle.
//       TextureIndex and MeshIndex are hardcoded array indeces that
//       point to an asset. They are only to be used to set up an asset
//       once in initialization. A handle is then generated which will
//       be the way the rest of the program references the asset.
//       TextureIndex and MeshIndex are to be replaced later with enums or
//       strings or tags or something more human readable but still hardcoded.
u32 GetTextureAssetHandle( u32 TextureIndex );
u32 GetMeshAssetHandle( u32 MeshIndex );
collider_mesh GetColliderMesh(u32 MeshHandle);
void RemoveRenderAsset( component_render* RenderComponent);
aabb3f GetMeshAABB(u32 MeshHandle);

inline game_asset_manager* GetAssetManager();

// Getters, Todo: Find a more elegant interface.
// Im isolating getters now because later the handle
// will not be the same as its array index.
// And I don't wanna change code in 200 different places
inline material*
GetMaterial(game_asset_manager* AssetManager, u32 Handle )
{
  Assert(Handle < AssetManager->MaterialCount);
  return AssetManager->Materials + Handle;
}
inline bitmap*
GetTexture(game_asset_manager* AssetManager, u32 Handle )
{
  Assert(Handle < AssetManager->TextureCount);
  return AssetManager->Textures + Handle;
}
inline book_keeper*
GetTextureKeeper(game_asset_manager* AssetManager, u32 Handle )
{
  Assert(Handle < AssetManager->TextureCount);
  return AssetManager->TextureKeeper + Handle;
}
inline mesh_indeces*
GetObject(game_asset_manager* AssetManager, u32 Handle )
{
  Assert(Handle < AssetManager->ObjectCount);
  return AssetManager->Objects + Handle;
}
inline book_keeper*
GetObjectKeeper(game_asset_manager* AssetManager, u32 Handle )
{
  Assert(Handle < AssetManager->ObjectCount);
  return AssetManager->ObjectKeeper + Handle;
}
inline mesh_data*
GetMeshData(game_asset_manager* AssetManager, u32 Handle )
{
  Assert(Handle < AssetManager->MeshCount);
  return AssetManager->MeshData + Handle;
}

enum MATERIAL_TYPE
{
  MATERIAL_WHITE,
  MATERIAL_RED,
  MATERIAL_GREEN,
  MATERIAL_BLUE,
  MATERIAL_EMERALD,
  MATERIAL_JADE,
  MATERIAL_OBSIDIAN,
  MATERIAL_PEARL,
  MATERIAL_RUBY,
  MATERIAL_TURQUOISE,
  MATERIAL_BRASS,
  MATERIAL_BRONZE,
  MATERIAL_CHROME,
  MATERIAL_COMPPER,
  MATERIAL_GOLD,
  MATERIAL_SILVER,
  MATERIAL_BLACK_PLASTIC,
  MATERIAL_CYAN_PLASTIC,
  MATERIAL_GREEN_PLASTIC,
  MATERIAL_RED_PLASTIC,
  MATERIAL_BLUE_PLASTIC,
  MATERIAL_WHITE_PLASTIC,
  MATERIAL_YELLOW_PLASTIC,
  MATERIAL_BLACK_RUBBER,
  MATERIAL_CYAN_RUBBER,
  MATERIAL_GREEN_RUBBER,
  MATERIAL_RED_RUBBER,
  MATERIAL_BLUE_RUBBER,
  MATERIAL_WHITE_RUBBER,
  MATERIAL_YELLOW_RUBBER
};
