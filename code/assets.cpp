#include "assets.h"
#include "obj_loader.h"

extern game_asset_manager* GlobalAssetManager;

void InitPredefinedMaterials(game_asset_manager* AssetManager)
{
  local_persist material mtl[30] =
  {
    { V4(1,        1,        1,        1), V4( 1,        1,          1,          1), V4( 1,          1,          1,          1), 1          }, // WHITE
    { V4(1,        0,        0,        1), V4( 1,        0,          0,          1), V4( 1,          0,          0,          1), 1          }, // RED
    { V4(0,        1,        0,        1), V4( 0,        1,          0,          1), V4( 0,          1,          0,          1), 1          }, // GREEN
    { V4(0,        0,        1,        1), V4( 0,        0,          1,          1), V4( 0,          0,          1,          1), 1          }, // Blue
    { V4(0.0215,   0.1745,   0.0215,   1), V4( 0.07568,  0.61424,    0.07568,    1), V4( 0.633,      0.727811,   0.633,      1), 0.6        }, // emerald
    { V4(0.135,    0.2225,   0.1575,   1), V4( 0.54,     0.89,       0.63,       1), V4( 0.316228,   0.316228,   0.316228,   1), 0.1        }, // jade
    { V4(0.05375,  0.05,     0.06625,  1), V4( 0.18275,  0.17,       0.22525,    1), V4( 0.332741,   0.328634,   0.346435,   1), 0.3        }, // obsidian
    { V4(0.25,     0.20725,  0.20725,  1), V4( 1.0,      0.829,      0.829,      1), V4( 0.296648,   0.296648,   0.296648,   1), 0.088      }, // pearl
    { V4(0.1745,   0.01175,  0.01175,  1), V4( 0.61424,  0.04136,    0.04136,    1), V4( 0.727811,   0.626959,   0.626959,   1), 0.6        }, // ruby
    { V4(0.1,      0.18725,  0.1745,   1), V4( 0.396,    0.74151,    0.69102,    1), V4( 0.297254,   0.30829,    0.306678,   1), 0.1        }, // turquoise
    { V4(0.329412, 0.223529, 0.027451, 1), V4( 0.780392, 0.568627,   0.113725,   1), V4( 0.992157,   0.941176,   0.807843,   1), 0.21794872 }, // brass
    { V4(0.2125,   0.1275,   0.054,    1), V4( 0.714,    0.4284,     0.18144,    1), V4( 0.393548,   0.271906,   0.166721,   1), 0.2        }, // bronze
    { V4(0.25,     0.25,     0.25,     1), V4( 0.4,      0.4,        0.4,        1), V4( 0.774597,   0.774597,   0.774597,   1), 0.6        }, // chrome
    { V4(0.19125,  0.0735,   0.0225,   1), V4( 0.7038,   0.27048,    0.0828,     1), V4( 0.256777,   0.137622,   0.086014,   1), 0.1        }, // copper
    { V4(0.24725,  0.1995,   0.0745,   1), V4( 0.75164,  0.60648,    0.22648,    1), V4( 0.628281,   0.555802,   0.366065,   1), 0.4        }, // gold
    { V4(0.19225,  0.19225,  0.19225,  1), V4( 0.50754,  0.50754,    0.50754,    1), V4( 0.508273,   0.508273,   0.508273,   1), 0.4        }, // silver
    { V4(0.0,      0.0,      0.0,      1), V4( 0.01,     0.01,       0.01,       1), V4( 0.50,       0.50,       0.50,       1), 0.25       }, // black plastic   
    { V4(0.0,      0.1,      0.06,     1), V4( 0.0,      0.50980392, 0.50980392, 1), V4( 0.50196078, 0.50196078, 0.50196078, 1), 0.25       }, // cyan plastic  
    { V4(0.0,      0.0,      0.0,      1), V4( 0.1,      0.35,       0.1,        1), V4( 0.45,       0.55,       0.45,       1), 0.25       }, // green plastic   
    { V4(0.0,      0.0,      0.0,      1), V4( 0.5,      0.0,        0.0,        1), V4( 0.7,        0.6,        0.6,        1), 0.25       }, // red plastic 
    { V4(0.0,      0.0,      0.0,      1), V4( 0.0,      0.0,        0.5,        1), V4( 0.6,        0.6,        0.7,        1), 0.25       }, // blue plastic      
    { V4(0.0,      0.0,      0.0,      1), V4( 0.55,     0.55,       0.55,       1), V4( 0.70,       0.70,       0.70,       1), 0.25       }, // white plastic   
    { V4(0.0,      0.0,      0.0,      1), V4( 0.5,      0.5,        0.0,        1), V4( 0.60,       0.60,       0.50,       1), 0.25       }, // yellow plastic
    { V4(0.02,     0.02,     0.02,     1), V4( 0.01,     0.01,       0.01,       1), V4( 0.4,        0.4,        0.4,        1), 0.078125   }, // black rubber  
    { V4(0.0,      0.05,     0.05,     1), V4( 0.4,      0.5,        0.5,        1), V4( 0.04,       0.7,        0.7,        1), 0.078125   }, // cyan rubber   
    { V4(0.0,      0.05,     0.0,      1), V4( 0.4,      0.5,        0.4,        1), V4( 0.04,       0.7,        0.04,       1), 0.078125   }, // green rubber  
    { V4(0.05,     0.0,      0.0,      1), V4( 0.5,      0.4,        0.4,        1), V4( 0.7,        0.04,       0.04,       1), 0.078125   }, // red rubber      
    { V4(0.00,     0.0,      0.05,     1), V4( 0.4,      0.4,        0.5,        1), V4( 0.04,       0.04,       0.7,        1), 0.078125   }, // blue rubber 
    { V4(0.05,     0.05,     0.05,     1), V4( 0.5,      0.5,        0.5,        1), V4( 0.7,        0.7,        0.7,        1), 0.078125   }, // white rubber  
    { V4(0.05,     0.05,     0.0,      1), V4( 0.5,      0.5,        0.4,        1), V4( 0.7,        0.7,        0.04,       1), 0.078125   } // yellow rubber        
  };

  Assert(AssetManager->MaterialCount + ArrayCount(mtl) < AssetManager->MaxMaterialCount);
  u8 WhitePixel[4] = {255,255,255,255};
  for(u32 MaterialIndex = 0; MaterialIndex < ArrayCount(mtl); ++MaterialIndex)
  {
    material* DstMaterial = (AssetManager->Materials + MaterialIndex);
    *DstMaterial = mtl[MaterialIndex];

    bitmap* EmptyBitmap = PushStruct(AssetManager->AssetArena, bitmap);
    EmptyBitmap->BPP  = 32;
    EmptyBitmap->Width  = 1;
    EmptyBitmap->Height = 1;
    EmptyBitmap->Pixels = PushCopy(AssetManager->AssetArena, sizeof(WhitePixel), WhitePixel);

    DstMaterial->DiffuseMap = EmptyBitmap;

    book_keeper* MaterialKeeper = (AssetManager->MaterialKeeper + MaterialIndex);
    *MaterialKeeper = {};

    ++AssetManager->MaterialCount;
  }
}


void InitiateAssetManager(game_state* State)
{

  if(!GlobalAssetManager)
  {
    State->AssetManager = PushStruct( &State->AssetArena, game_asset_manager);
    State->AssetManager->AssetArena = &State->AssetArena;

    State->AssetManager->MaxMeshCount = 64;
    State->AssetManager->MeshCount = 0;
    State->AssetManager->MeshData = PushArray(State->AssetManager->AssetArena,
                                              State->AssetManager->MaxMeshCount, mesh_data);

    State->AssetManager->MaxObjectCount = 64;
    State->AssetManager->ObjectCount = 0;
    State->AssetManager->Objects = PushArray(State->AssetManager->AssetArena,
                                              State->AssetManager->MaxObjectCount, mesh_indeces);
    State->AssetManager->ObjectKeeper = PushArray(State->AssetManager->AssetArena,
                                              State->AssetManager->MaxObjectCount, book_keeper);

    State->AssetManager->MaxMaterialCount = 64;
    State->AssetManager->MaterialCount = 0;
    State->AssetManager->Materials = PushArray( State->AssetManager->AssetArena,
                                              State->AssetManager->MaxMaterialCount, material);
    State->AssetManager->MaterialKeeper = PushArray(State->AssetManager->AssetArena,
                                              State->AssetManager->MaxMaterialCount, book_keeper);

    InitPredefinedMaterials(State->AssetManager);

    GlobalAssetManager = State->AssetManager;
  }
}

static inline game_asset_manager*
GetAssetManager()
{
  Assert(GlobalAssetManager);
  return GlobalAssetManager;
}

// TODO: Create a global_context and stuff the Transient arena in there.
void LoadCubeAsset( game_state* State)
{
  memory_arena* AssetArena = &State->AssetArena;

  game_asset_manager* AssetManager = GetAssetManager();

  obj_loaded_file* LoadedObjFile = ReadOBJFile( AssetArena, &State->TransientArena,
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

  material* DstMaterial = AssetManager->Materials + AssetManager->MaterialCount;
  AssetManager->MaterialCount += LoadedObjFile->MaterialData->MaterialCount;
  for (u32 i = 0; i < LoadedObjFile->MaterialData->MaterialCount; ++i)
  {
    mtl_material* SrcMaterial = LoadedObjFile->MaterialData->Materials + i;
    DstMaterial->AmbientColor  = SrcMaterial->Ka ? *SrcMaterial->Ka : V4(1,1,1,1);
    DstMaterial->DiffuseColor  = SrcMaterial->Kd ? *SrcMaterial->Kd : V4(1,1,1,1);
    DstMaterial->SpecularColor = SrcMaterial->Ks ? *SrcMaterial->Ks : V4(1,1,1,1);
    DstMaterial->Shininess     = SrcMaterial->Ns ? *SrcMaterial->Ns : 1;
    DstMaterial->DiffuseMap    = SrcMaterial->MapKd;
    ++AssetManager->MaterialCount;
    DstMaterial++;
  }
}

u32 GetMeshAssetHandle( u32 MeshIndex )
{
  game_asset_manager* AssetManager = GetAssetManager();
  Assert(MeshIndex < AssetManager->ObjectCount);
  mesh_indeces* Object = AssetManager->Objects + MeshIndex;
  AssetManager->ObjectKeeper[MeshIndex].ReferenceCount++;
  Assert(Object->MeshIndex < AssetManager->MeshCount);
  return MeshIndex;
}

aabb3f GetMeshAABB(u32 MeshHandle)
{
  game_asset_manager* AssetManager = GetAssetManager();
  Assert(MeshHandle < AssetManager->ObjectCount);
  aabb3f Result = (AssetManager->Objects+MeshHandle)->AABB;
  return Result;
}

collider_mesh GetColliderMesh(u32 MeshHandle)
{
  collider_mesh Result = {};
  game_asset_manager* AssetManager = GetAssetManager();
  mesh_indeces* Indeces =  AssetManager->Objects + MeshHandle;
  mesh_data* Data =  AssetManager->MeshData + Indeces->MeshIndex;
  Result.nv = Data->nv;
  Result.v = Data->v;
  Result.nvi = Indeces->Count;
  Result.vi = Indeces->vi;
  return Result;
}
u32 GetTextureAssetHandle( u32 TextureIndex )
{
  game_asset_manager* AssetManager = GetAssetManager();
  Assert(TextureIndex < AssetManager->MaterialCount);
  material* Material = AssetManager->Materials + TextureIndex;
  AssetManager->MaterialKeeper[TextureIndex].ReferenceCount++;
  return TextureIndex;
}

void RemoveMeshAsset(u32 MeshHandle)
{
  game_asset_manager* AssetManager = GetAssetManager();
  Assert(MeshHandle < AssetManager->ObjectCount);
  mesh_indeces* Object = AssetManager->Objects + MeshHandle;
  AssetManager->ObjectKeeper[MeshHandle].ReferenceCount--;
}
void RemoveTextureAsset(u32 TextureHandle)
{
  game_asset_manager* AssetManager = GetAssetManager();
  Assert(TextureHandle < AssetManager->MaterialCount);
  material* Material = AssetManager->Materials + TextureHandle;
  AssetManager->MaterialKeeper[TextureHandle].ReferenceCount--;
}