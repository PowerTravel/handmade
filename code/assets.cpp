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
  if(PixelData)
  {
    Bitmap->Pixels = PushCopy(&AssetManager->AssetArena, MemorySize, PixelData);
  }else{
    Bitmap->Pixels = PushSize(&AssetManager->AssetArena, MemorySize);
  }
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

void GetHandle(game_asset_manager* AssetManager, char* Key, bitmap_handle* Handle)
{
  Assert(Key && Handle);
  Handle->Value = GetBitmapAssetIndex(AssetManager, Key);

  bitmap_keeper* BitmapKeeper =  AssetManager->BitmapKeeper + Handle->Value;
  if(!BitmapKeeper->Referenced)
  {
    BitmapKeeper->Referenced = true;
    AssetManager->BitmapPendingLoad[AssetManager->BitmapPendingLoadCount++] = *Handle;
  }
}

void Reupload(game_asset_manager* AssetManager, bitmap_handle Handle, rect2f SubRegion)
{
  bitmap_keeper* BitmapKeeper =  AssetManager->BitmapKeeper + Handle.Value;
  BitmapKeeper->Referenced = true;
  BitmapKeeper->UseSubRegion = true;
  BitmapKeeper->SubRegion = SubRegion;
  AssetManager->BitmapPendingLoad[AssetManager->BitmapPendingLoadCount++] = Handle;
}

void Reupload(game_asset_manager* AssetManager, bitmap_handle Handle)
{
  bitmap_keeper* BitmapKeeper = AssetManager->BitmapKeeper + Handle.Value;
  BitmapKeeper->Referenced = true;
  BitmapKeeper->UseSubRegion = false;
  AssetManager->BitmapPendingLoad[AssetManager->BitmapPendingLoadCount++] = Handle;
}

void GetHandle(game_asset_manager* AssetManager, char* Key, object_handle* Handle)
{
  Assert(Key && Handle);
  Handle->Value = GetObjectAssetIndex(AssetManager, Key);

  buffer_keeper* ObjectKeeper = AssetManager->ObjectKeeper + Handle->Value;
  if(!ObjectKeeper->Referenced)
  {
    ObjectKeeper->Referenced = true;
    AssetManager->ObjectPendingLoad[AssetManager->ObjectPendingLoadCount++] = *Handle;
  }
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


  buffer_keeper* ObjectKeeper = AssetManager->ObjectKeeper + Handle.Value;
  Assert(ObjectKeeper->Referenced);
  if(Keeper) *Keeper = ObjectKeeper;

  return Result;
}
inline mesh_data*
GetAsset(game_asset_manager* AssetManager, mesh_handle Handle)//, buffer_keeper** Keeper)
{
  Assert(Handle.Value < AssetManager->Meshes.Count);
  mesh_data** MeshData = (mesh_data**) AssetManager->Meshes.Values;
  mesh_data* Result = MeshData[Handle.Value];

 // if(Keeper) *Keeper = AssetManager->MeshKeeper + Handle.Value;

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

  bitmap_keeper* BitmapKeeper = AssetManager->BitmapKeeper + Handle.Value;
  Assert(BitmapKeeper->Referenced);
  if(Keeper) *Keeper = BitmapKeeper;

  return Result;
}


collider_mesh GetColliderMesh( game_asset_manager* AssetManager, object_handle Handle)
{
  mesh_indeces* Object = GetAsset(AssetManager, Handle);
  mesh_data* Data = GetAsset(AssetManager, Object->MeshHandle);
  collider_mesh Result = {};
  Result.nv = Data->nv;
  Result.v = Data->v;
  Result.nvi = Object->Count;
  Result.vi = Object->vi;
  return Result;
}

aabb3f GetMeshAABB(game_asset_manager* AssetManager, object_handle Handle)
{
  mesh_indeces* Object = GetAsset(AssetManager, Handle);
  return Object->AABB;
}

inline object_handle
GetEnumeratedObjectHandle(game_asset_manager* AssetManager, predefined_mesh MeshType)
{
  object_handle Result = AssetManager->EnumeratedMeshes[(u32)MeshType];
  return Result;
}

