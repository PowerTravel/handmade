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

internal bitmap_handle
PushBitmapData(game_asset_manager* AssetManager, c8* Key, u32 Width, u32 Height, u32 BPP, void* PixelData, b32 IsSpecial)
{
  bitmap_handle Handle = {};
  Handle.Value = GetBitmapAssetIndex(AssetManager, Key);
  SetKey(&AssetManager->AssetArena, &AssetManager->Bitmaps, Handle.Value, Key);
  bitmap* Bitmap = AllocateBitmap(AssetManager, Handle.Value);
  Bitmap->BPP = BPP;
  Bitmap->Special = IsSpecial;
  Bitmap->Width = Width;
  Bitmap->Height = Height;
  midx MemorySize = Width * Height * BPP / 8;
  Bitmap->Pixels = PushCopy(&AssetManager->AssetArena, MemorySize, PixelData);
  return Handle;
}

internal mesh_handle
PushMeshData(game_asset_manager* AssetManager, u32 nv, v3* v, u32 nvn, v3* vn, u32 nvt, v2* vt)
{
  mesh_handle Handle = {};
  Handle.Value = AssetManager->Meshes.Count;
  Assert(AssetManager->Meshes.Count < AssetManager->Meshes.MaxCount);
  mesh_data* Data = AllocateMesh(AssetManager, Handle.Value);
  Data->nv  = nv;    // Nr Verices
  Data->nvn = nvn;   // Nr Vertice Normals
  Data->nvt = nvt;   // Nr Trxture Vertices
  Data->v  = (v3*) PushCopy(&AssetManager->AssetArena, nv*sizeof(v3), v);
  Data->vn = (v3*) PushCopy(&AssetManager->AssetArena, nvn*sizeof(v3), vn);
  Data->vt = (v2*) PushCopy(&AssetManager->AssetArena, nvt*sizeof(v2), vt);
  return Handle;
}

internal object_handle
PushIndexData(game_asset_manager* AssetManager, c8* Key, mesh_handle MeshHandle, u32 Count, u32* vi, u32* ti, u32* ni, aabb3f AABB)
{
  object_handle Handle = {};

  Handle.Value = GetObjectAssetIndex(AssetManager, Key);
  SetKey(&AssetManager->AssetArena, &AssetManager->Objects, Handle.Value, Key);
  mesh_indeces* Indeces = AllocateObject(AssetManager, Handle.Value);
  Indeces->MeshHandle = MeshHandle;
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
  return Handle;
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
instance_handle GetAssetHandle(game_asset_manager* AssetManager)
{
  instance_handle Result = {};
  Assert(AssetManager->TemporaryInstancesBase == 0);
  Result.Value = AssetManager->Instances.Count;
  AllocateInstance(AssetManager, Result.Value);
  return Result;
}

void ResetAssetManagerTemporaryInstances(game_asset_manager* AssetManager)
{
  AssetManager->TemporaryInstancesCount = 0;
}

instance_handle GetTemporaryAssetHandle(game_asset_manager* AssetManager)
{
  if(AssetManager->TemporaryInstancesBase == 0)
  {
    AssetManager->TemporaryInstancesBase = AssetManager->Instances.Count;
    AssetManager->TemporaryInstancesCount = 0;
  }

  u32 Base = AssetManager->TemporaryInstancesBase;
  u32 Count = AssetManager->TemporaryInstancesCount++;
  u32 AssetInstanceIndex = Base + Count;
  
  asset_vector* Instances = &AssetManager->Instances;
  Assert(AssetInstanceIndex < Instances->MaxCount);
  asset_instance* Instance = (asset_instance*) Instances->Values[AssetInstanceIndex];
  if(!Instance)
  {
    Instances->Values[AssetInstanceIndex] = PushStruct(&AssetManager->AssetArena, asset_instance);
  }else{
    *Instance = {};
  }

  instance_handle Result = {};
  Result.Value = AssetInstanceIndex;
  return Result;
}

void GetHandle(game_asset_manager* AssetManager, char* Key, bitmap_handle* Handle)
{
  Assert(Key && Handle);
  Handle->Value = GetBitmapAssetIndex(AssetManager, Key);
}
void GetHandle(game_asset_manager* AssetManager, char* Key, object_handle* Handle)
{
  Assert(Key && Handle);
  Handle->Value = GetObjectAssetIndex(AssetManager, Key);
}
void GetHandle(game_asset_manager* AssetManager, char* Key, material_handle* Handle)
{
  Assert(Key && Handle);
  Handle->Value = GetMaterialAssetIndex(AssetManager, Key);
}


inline mesh_indeces*
GetAsset(game_asset_manager* AssetManager, object_handle Handle, buffer_keeper** Keeper)
{
  Assert(Handle.Value < AssetManager->Objects.MaxCount);
  mesh_indeces** Objects = (mesh_indeces**) AssetManager->Objects.Values;
  mesh_indeces* Result = Objects[Handle.Value];
  Assert(Result);

  if(Keeper) *Keeper = AssetManager->ObjectKeeper + Handle.Value;

  return Result;
}
inline mesh_data*
GetAsset(game_asset_manager* AssetManager, mesh_handle Handle, buffer_keeper** Keeper)
{
  Assert(Handle.Value < AssetManager->Meshes.Count);
  mesh_data** MeshData = (mesh_data**) AssetManager->Meshes.Values;
  mesh_data* Result = MeshData[Handle.Value];

  if(Keeper) *Keeper = AssetManager->MeshKeeper + Handle.Value;

  Assert(Result);
  return Result;
}
inline material*
GetAsset(game_asset_manager* AssetManager, material_handle Handle)
{
  Assert(Handle.Value < AssetManager->Materials.MaxCount);
  material** Materials = (material**) AssetManager->Materials.Values;
  material* Result = Materials[Handle.Value];
  Assert(Result);
  return Result;
}
inline bitmap*
GetAsset(game_asset_manager* AssetManager, bitmap_handle Handle, bitmap_keeper** Keeper)
{
  Assert(Handle.Value < AssetManager->Bitmaps.MaxCount);
  bitmap** Bitmaps = (bitmap**) AssetManager->Bitmaps.Values;
  bitmap* Result = Bitmaps[Handle.Value];

  Assert(Result);

  if(Keeper) *Keeper = AssetManager->BitmapKeeper + Handle.Value;

  return Result;
}

internal inline asset_instance*
GetAssetInstance(game_asset_manager* AssetManager, instance_handle Handle)
{
  Assert(Handle.Value < AssetManager->Instances.Count + AssetManager->TemporaryInstancesCount);
  asset_instance** Instances = (asset_instance**) AssetManager->Instances.Values;
  asset_instance* Result = Instances[Handle.Value];
  Assert(Result);
  return Result;
}

inline mesh_indeces*
GetObject(game_asset_manager* AssetManager, instance_handle Handle, buffer_keeper** Keeper)
{
  asset_instance* Instance = GetAssetInstance(AssetManager, Handle);
  mesh_indeces* Result = GetAsset(AssetManager, Instance->ObjectHandle, Keeper);
  return Result;
};
inline mesh_data*
GetMesh(game_asset_manager* AssetManager, instance_handle Handle, buffer_keeper** Keeper)
{
  mesh_indeces* Object = GetObject(AssetManager, Handle );
  mesh_data* Result = GetAsset(AssetManager, Object->MeshHandle, Keeper);
  return Result;
}

collider_mesh GetColliderMesh( game_asset_manager* AssetManager, instance_handle Handle)
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

aabb3f GetMeshAABB(game_asset_manager* AssetManager, instance_handle Handle)
{
  mesh_indeces* Object = GetObject(AssetManager, Handle);
  return Object->AABB;
}

bitmap* GetBitmap(game_asset_manager* AssetManager, instance_handle Handle, bitmap_keeper** Keeper)
{
  asset_instance* Instance = GetAssetInstance(AssetManager, Handle);
  bitmap* Result = GetAsset(AssetManager, Instance->BitmapHandle, Keeper);
  return Result;
}

material* GetMaterial(game_asset_manager* AssetManager, instance_handle Handle )
{
  asset_instance* Instance = GetAssetInstance(AssetManager, Handle);
  material* Result = GetAsset(AssetManager, Instance->MaterialHandle );
  return Result;
};

void SetAsset(game_asset_manager* AssetManager, asset_type AssetType, char* Name, instance_handle Handle)
{
  asset_instance* Instance = GetAssetInstance(AssetManager, Handle);
  switch(AssetType)
  {
    case asset_type::OBJECT:
    {
      Instance->ObjectHandle.Value = GetObjectAssetIndex(AssetManager, Name);

      buffer_keeper* ObjectKeeper = 0;
      mesh_indeces* Indeces = GetAsset(AssetManager, Instance->ObjectHandle, &ObjectKeeper);

      u32 InitialObjectCount = ObjectKeeper->ReferenceCount++;
      if(InitialObjectCount == 0)
      {
        AssetManager->ObjectPendingLoad[AssetManager->ObjectPendingLoadCount++] = Instance->ObjectHandle;
      }

//      buffer_keeper* MeshKeeper = 0;
//      GetAsset(AssetManager, Indeces->MeshHandle, &MeshKeeper);
//      u32 InitialMeshCount = MeshKeeper->ReferenceCount++;
//      if(InitialMeshCount == 0)
//      {
//        AssetManager->MeshPendingLoad[AssetManager->MeshPendingLoadCount++] = Indeces->MeshHandle;
//      }

      Assert(IsKeySet(&AssetManager->Objects, Instance->ObjectHandle.Value));
    }break;
    case asset_type::BITMAP:
    {
      Instance->BitmapHandle.Value = GetBitmapAssetIndex(AssetManager, Name);

      bitmap_keeper* Keeper = 0;
      GetAsset(AssetManager, Instance->BitmapHandle, &Keeper);
      u32 InitialCount = Keeper->ReferenceCount++;
      if(InitialCount == 0)
      {
        AssetManager->BitmapPendingLoad[AssetManager->BitmapPendingLoadCount++] = Instance->BitmapHandle;
      }
      Assert(IsKeySet(&AssetManager->Bitmaps, Instance->BitmapHandle.Value));
    }break;
    case asset_type::MATERIAL:
    {
      Instance->MaterialHandle.Value = GetMaterialAssetIndex(AssetManager, Name);
      Assert(IsKeySet(&AssetManager->Materials, Instance->MaterialHandle.Value));
    }break;

  }
}

inline object_handle
GetEnumeratedObjectHandle(game_asset_manager* AssetManager, predefined_mesh MeshType)
{
  object_handle Result = AssetManager->EnumeratedMeshes[(u32)MeshType];
  return Result;
}

