#include "assets.h"
#include "obj_loader.h"

void LoadCubeAsset( game_state* State  )
{
  memory_arena* AssetArena = &State->AssetArena;
  memory_arena* PersistentArena = &State->PersistentArena;
  memory_arena* TransientArena = &State->TransientArena;
  world* World = State->World;

  game_asset_manager* AssetManager = State->AssetManager;
  if(!AssetManager)
  {
    State->AssetManager =  PushStruct(AssetArena, game_asset_manager);
    AssetManager =  State->AssetManager;
  }

  obj_loaded_file* LoadedObjFile = ReadOBJFile( AssetArena, TransientArena,
   Platform.DEBUGPlatformReadEntireFile,
   Platform.DEBUGPlatformFreeFileMemory,
   "..\\handmade\\data\\cube\\cube.obj");

  AssetManager->MeshCount = 1;
  AssetManager->MeshData = PushArray(AssetArena,AssetManager->MeshCount, mesh_data);
  *AssetManager->MeshData =*LoadedObjFile->MeshData;

  AssetManager->ObjectCount = LoadedObjFile->ObjectCount;
  AssetManager->Objects = PushArray(AssetArena, AssetManager->ObjectCount, mesh_indeces);
  AssetManager->ObjectKeeper = PushArray(AssetArena, AssetManager->ObjectCount, book_keeper);
  mesh_indeces* DstObject = AssetManager->Objects;
  for (u32 i = 0; i < LoadedObjFile->ObjectCount; ++i)
  {
    obj_group* LoadedObjectGroup = LoadedObjFile->Objects + i;
    DstObject->MeshIndex = 0; // MeshDataIndex
    DstObject->AABB = LoadedObjectGroup->aabb;
    DstObject->Count = LoadedObjectGroup->Indeces->Count;  // 3 times Nr Triangles
    DstObject->vi = LoadedObjectGroup->Indeces->vi;    // Vertex Indeces
    DstObject->ti = LoadedObjectGroup->Indeces->ti;    // Texture Indeces
    DstObject->ni = LoadedObjectGroup->Indeces->ni;    // Normal Indeces
    ++DstObject;
  }

  AssetManager->MaterialCount = LoadedObjFile->MaterialData->MaterialCount;
  AssetManager->Materials = PushArray( AssetArena, AssetManager->MaterialCount, material);
  AssetManager->MaterialKeeper = PushArray(AssetArena, AssetManager->MaterialCount, book_keeper);
  material* DstMaterial = AssetManager->Materials;
  for (u32 i = 0; i < AssetManager->MaterialCount; ++i)
  {
    mtl_material* SrcMaterial = LoadedObjFile->MaterialData->Materials + i;
    DstMaterial->AmbientColor  = SrcMaterial->Ka ? *SrcMaterial->Ka : V4(1,1,1,1);
    DstMaterial->DiffuseColor  = SrcMaterial->Kd ? *SrcMaterial->Kd : V4(1,1,1,1);
    DstMaterial->SpecularColor = SrcMaterial->Ks ? *SrcMaterial->Ks : V4(1,1,1,1);
    DstMaterial->Shininess     = SrcMaterial->Ns ? *SrcMaterial->Ns : 1;
    DstMaterial->DiffuseMap    = SrcMaterial->MapKd;
    DstMaterial++;
  }
}

void SetRenderAsset(game_asset_manager* AssetManager, component_render* RenderComponent, u32 ObjectIndex, u32 TextureIndex )
{
  Assert(ObjectIndex < AssetManager->ObjectCount);
  mesh_indeces* Object = AssetManager->Objects + ObjectIndex;
  AssetManager->ObjectKeeper[ObjectIndex].ReferenceCount++;
  Assert(Object->MeshIndex < AssetManager->MeshCount);

  Assert(TextureIndex < AssetManager->MaterialCount);
  material* Material = AssetManager->Materials + TextureIndex;
  AssetManager->MaterialKeeper[TextureIndex].ReferenceCount++;

  RenderComponent->Object = ObjectIndex;
  RenderComponent->Texture = TextureIndex;
}

void RemoveRenderAsset(game_asset_manager* AssetManager, component_render* RenderComponent)
{
  Assert(RenderComponent->Object < AssetManager->ObjectCount);
  mesh_indeces* Object = AssetManager->Objects + RenderComponent->Object;
  AssetManager->ObjectKeeper[RenderComponent->Object].ReferenceCount--;

  Assert(RenderComponent->Texture < AssetManager->MaterialCount);
  material* Material = AssetManager->Materials + RenderComponent->Texture;
  AssetManager->MaterialKeeper[RenderComponent->Texture].ReferenceCount--;
}