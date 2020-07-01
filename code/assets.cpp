#include "assets.h"
#include "obj_loader.h"
#include "utility_macros.h"

void* AllocateValue_(memory_arena* Arena, midx Size, void** Values, u32 Index, u32* Count, u32* MaxCount)
{
  Assert(*Count < *MaxCount);
  void* Result = Values[Index];
  if (Arena && !Result)
  {
    (*Count)++;
    Assert(*Count < *MaxCount);
    Result = PushSize(Arena, Size);
    Values[Index] = Result;
  }

  return Result;
}

#define AllocateInstance(AssetManager, Index) (asset_instance*) AllocateValue_(&AssetManager->AssetArena, sizeof(asset_instance), AssetManager->Instances.Values, Index, &AssetManager->Instances.Count, &AssetManager->Instances.MaxCount);
#define AllocateMesh(AssetManager, Index) (mesh_data*) AllocateValue_(&AssetManager->AssetArena, sizeof(mesh_data), AssetManager->Meshes.Values, Index, &AssetManager->Meshes.Count, &AssetManager->Meshes.MaxCount);
#define AllocateBitmap(AssetManager, Index) (bitmap*) AllocateValue_(&AssetManager->AssetArena, sizeof(bitmap), AssetManager->Bitmaps.Values, Index, &AssetManager->Bitmaps.Count, &AssetManager->Bitmaps.MaxCount);
#define AllocateObject(AssetManager, Index) (mesh_indeces*) AllocateValue_(&AssetManager->AssetArena, sizeof(mesh_indeces), AssetManager->Objects.Values, Index, &AssetManager->Objects.Count, &AssetManager->Objects.MaxCount);
#define AllocateMaterial(AssetManager, Index) (material*) AllocateValue_(&AssetManager->AssetArena, sizeof(material), AssetManager->Materials.Values, Index, &AssetManager->Materials.Count, &AssetManager->Materials.MaxCount);

u32 StringToIndex(asset_hash_map* HashMap, c8* Key)
{
  Assert(Key);
  Assert(str::StringLength(Key) > 0);

  const u32 StartIndex = utils::djb2_hash(Key) % HashMap->MaxCount;

  u32 Index = StartIndex;
  c8* MapKey = HashMap->Keys[StartIndex];
  while(MapKey && !str::Equals(Key, MapKey))
  {
    ++Index;
    Index = Index % HashMap->MaxCount;
    MapKey = HashMap->Keys[Index];
    Assert(Index != StartIndex);
  }
  return Index;
}

internal inline b32
IsKeySet(asset_hash_map* HashMap, u32 Index)
{
  return HashMap->Keys[Index] != nullptr;
}

internal inline void
SetKey(memory_arena* Arena, asset_hash_map* HashMap, u32 Index, c8* Key)
{
  Assert(Key);
  Assert(!IsKeySet(HashMap, Index));
  c8* dst = (c8*) PushCopy(Arena, str::StringLength(Key)*sizeof(c8), Key);
  HashMap->Keys[Index] = dst;
}

internal inline u32
GetObjectAssetIndex(game_asset_manager* AssetManager, c8* Key)
{
  u32 Result = StringToIndex(&AssetManager->Objects, Key);
  return Result;
}
internal inline u32
GetMaterialAssetIndex(game_asset_manager* AssetManager, c8* Key)
{
  u32 Result = StringToIndex(&AssetManager->Materials, Key);
  return Result;
}
internal inline u32
GetBitmapAssetIndex(game_asset_manager* AssetManager, c8* Key)
{
  u32 Result = StringToIndex(&AssetManager->Bitmaps, Key);
  return Result;
}

internal u32
PushBitmapData(game_asset_manager* AssetManager, c8* Key, u32 Width, u32 Height, u32 BPP, void* PixelData)
{
  u32 Index = GetBitmapAssetIndex(AssetManager, Key);
  SetKey(&AssetManager->AssetArena, &AssetManager->Bitmaps, Index, Key);
  bitmap* Bitmap = AllocateBitmap(AssetManager, Index);
  Bitmap->BPP = BPP;
  Bitmap->Width = Width;
  Bitmap->Height = Height;
  midx MemorySize = Width * Height * BPP / 8;
  Bitmap->Pixels = PushCopy(&AssetManager->AssetArena, MemorySize, PixelData);
  return Index;
}

internal u32
PushMeshData(game_asset_manager* AssetManager, u32 nv, v3* v, u32 nvn, v3* vn, u32 nvt, v2* vt)
{
  u32 Index = AssetManager->Meshes.Count;
  Assert(AssetManager->Meshes.Count < AssetManager->Meshes.MaxCount)
  mesh_data* Data = AllocateMesh(AssetManager, Index);
  Data->nv  = nv;    // Nr Verices
  Data->nvn = nvn;   // Nr Vertice Normals
  Data->nvt = nvt;   // Nr Trxture Vertices
  Data->v  = (v3*) PushCopy(&AssetManager->AssetArena, nv*sizeof(v3), v);
  Data->vn = (v3*) PushCopy(&AssetManager->AssetArena, nvn*sizeof(v3), vn);
  Data->vt = (v2*) PushCopy(&AssetManager->AssetArena, nvt*sizeof(v2), vt);
  return Index;
}

internal u32
PushIndexData(game_asset_manager* AssetManager, c8* Key, u32 MeshIndex, u32 Count, u32* vi, u32* ti, u32* ni, aabb3f AABB)
{
  u32 Index = GetObjectAssetIndex(AssetManager, Key);
  SetKey(&AssetManager->AssetArena, &AssetManager->Objects, Index, Key);
  mesh_indeces* Indeces = AllocateObject(AssetManager, Index);
  Indeces->MeshHandle = MeshIndex;
  Indeces->Count  = Count;
  Indeces->vi = (u32*) PushCopy(&AssetManager->AssetArena, Count * sizeof(u32), vi);
  if(ti)
  {
    Indeces->ti = (u32*) PushCopy(&AssetManager->AssetArena, Count * sizeof(u32), ti);
  }
  if(ni)
  {
    Indeces->ni = (u32*) PushCopy(&AssetManager->AssetArena, Count * sizeof(u32), ni);
  }
  Indeces->AABB = AABB;
  str::CopyStrings( str::StringLength( Key ), Key,
                    ArrayCount( Indeces->Name ), Indeces->Name );
  return Index;
}

internal void
PushMaterialData(game_asset_manager* AssetManager, c8* Key, material SrcMaterial)
{
  u32 Index = GetMaterialAssetIndex(AssetManager, Key);
  SetKey(&AssetManager->AssetArena, &AssetManager->Materials, Index, Key);
  material* DstMaterial = AllocateMaterial(AssetManager, Index);
  *DstMaterial = SrcMaterial;
}

// Game Layer API
u32 GetAssetHandle(game_asset_manager* AssetManager)
{
  u32 Result = AssetManager->Instances.Count;
  AllocateInstance(AssetManager, Result);
  return Result;
}

void ResetAssetManagerTemporaryInstances(game_asset_manager* AssetManager)
{
  AssetManager->TemporaryInstancesCount = 0;
}

u32 GetTemporaryAssetHandle(game_asset_manager* AssetManager)
{
  if(AssetManager->TemporaryInstancesBase == 0)
  {
    AssetManager->TemporaryInstancesBase = AssetManager->Instances.Count;
    AssetManager->TemporaryInstancesCount = 0;
  }

  u32 Base = AssetManager->TemporaryInstancesBase;
  u32 Count = AssetManager->TemporaryInstancesCount++;
  u32 Result = Base+Count;

  asset_vector* Instances = &AssetManager->Instances;
  Assert(Result < Instances->MaxCount);
  asset_instance* Instance = (asset_instance*) Instances->Values[Result];
  if(!Instance)
  {
    Instances->Values[Result] = PushStruct(&AssetManager->AssetArena, asset_instance);
  }else{
    *Instance = {};
  }

  return Result;
}

void CopyAssets(game_asset_manager* AssetManager, u32 SrcHandle, u32 DstHandle)
{
  Assert(SrcHandle < AssetManager->Instances.Count);
  Assert(DstHandle < AssetManager->Instances.Count+AssetManager->TemporaryInstancesCount);

  utils::Copy(sizeof(asset_instance), AssetManager->Instances.Values[SrcHandle],
                                      AssetManager->Instances.Values[DstHandle]);
}

u32 GetAssetIndex(game_asset_manager* AssetManager, asset_type AssetType, char* Key)
{
  u32 Result =  U32Max;
  switch(AssetType)
  {
    case asset_type::OBJECT:
    {
      Result = GetObjectAssetIndex( AssetManager, Key);
      Assert(IsKeySet(&AssetManager->Objects, Result));
    }break;
    case asset_type::MATERIAL:
    {
      Result = GetMaterialAssetIndex(AssetManager, Key);
      Assert(IsKeySet(&AssetManager->Materials, Result));
    }break;
    case asset_type::BITMAP:
    {
      Result = GetBitmapAssetIndex(AssetManager, Key);
      Assert(IsKeySet(&AssetManager->Bitmaps, Result));
    }break;
  }

  Assert(Result != U32Max);
  return Result;
}


void SetAsset(game_asset_manager* AssetManager, asset_type AssetType, char* Key, u32 Handle)
{
  asset_instance* Instance = GetAssetInstance(AssetManager, Handle);
  switch(AssetType)
  {
    case asset_type::OBJECT:
    {
      Instance->ObjectIndex = GetObjectAssetIndex(AssetManager, Key);
      Assert(IsKeySet(&AssetManager->Objects, Instance->ObjectIndex));
    }break;
    case asset_type::MATERIAL:
    {
      Instance->MaterialIndex = GetMaterialAssetIndex(AssetManager, Key);
      Assert(IsKeySet(&AssetManager->Materials, Instance->MaterialIndex));
    }break;
    case asset_type::BITMAP:
    {
      Instance->TextureIndex = GetBitmapAssetIndex(AssetManager, Key);
      Assert(IsKeySet(&AssetManager->Bitmaps, Instance->TextureIndex));
    }break;
  }
}