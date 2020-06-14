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

inline b32 IsKeySet(asset_hash_map* HashMap, u32 Index)
{
  return HashMap->Keys[Index] != nullptr;
}

internal inline u32 GetNamedHandle(asset_hash_map* HashMap,  c8* Key)
{
  Assert(Key);
  u32 Result = StringToIndex(HashMap, Key);
  Assert(IsKeySet(HashMap, Result));
  return Result;
}

internal void SetKey(memory_arena* Arena, asset_hash_map* HashMap, u32 Index, c8* Key)
{
  Assert(Key);
  Assert(!IsKeySet(HashMap, Index));
  c8* dst = (c8*) PushCopy(Arena, str::StringLength(Key)*sizeof(c8), Key);
  HashMap->Keys[Index] = dst;
}

inline u32 GetObjectAssetIndex(game_asset_manager* AssetManager, c8* Key)
{
  u32 Result = StringToIndex(&AssetManager->Objects, Key);
  return Result;  
}
inline u32 GetMaterialAssetIndex(game_asset_manager* AssetManager, c8* Key)
{
  u32 Result = StringToIndex(&AssetManager->Materials, Key);
  return Result;
}
inline u32 GetBitmapAssetIndex(game_asset_manager* AssetManager, c8* Key)
{
  u32 Result = StringToIndex(&AssetManager->Bitmaps, Key);
  return Result;
}

internal u32 PushBitmapData(game_asset_manager* AssetManager, c8* Key, u32 Width, u32 Height, u32 BPP, void* PixelData)
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

internal u32 PushMeshData(game_asset_manager* AssetManager, u32 nv, v3* v, u32 nvn, v3* vn, u32 nvt, v2* vt)
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

internal u32 PushIndexData(game_asset_manager* AssetManager, c8* Key, u32 MeshIndex, u32 Count, u32* vi, u32* ti, u32* ni, aabb3f AABB)
{
  u32 Index = GetObjectAssetIndex(AssetManager, Key);
  SetKey(&AssetManager->AssetArena, &AssetManager->Objects, Index, Key);
  mesh_indeces* Indeces = AllocateObject(AssetManager, Index);
  Indeces->MeshHandle = MeshIndex;
  Indeces->Count  = Count;
  Indeces->vi = (u32*) PushCopy(&AssetManager->AssetArena, Count * sizeof(u32), vi);
  Indeces->ti = (u32*) PushCopy(&AssetManager->AssetArena, Count * sizeof(u32), ti);
  Indeces->ni = (u32*) PushCopy(&AssetManager->AssetArena, Count * sizeof(u32), ni);
  Indeces->AABB = AABB;
  str::CopyStrings( str::StringLength( Key ), Key,
                    ArrayCount( Indeces->Name ), Indeces->Name );
  return Index;
}

internal void PushMaterialData(game_asset_manager* AssetManager, c8* Key, material SrcMaterial)
{
  u32 Index = GetMaterialAssetIndex(AssetManager, Key);
  SetKey(&AssetManager->AssetArena, &AssetManager->Materials, Index, Key);
  material* DstMaterial = AllocateMaterial(AssetManager, Index);
  *DstMaterial = SrcMaterial;
}

void LoadPredefinedMaterials(game_asset_manager* AssetManager)
{ 
  u8 WhitePixel[4] = {255,255,255,255};
  u32 BitmapIndex = PushBitmapData(AssetManager, "null", 1, 1, 32, (void*)WhitePixel);

  PushMaterialData(AssetManager, "white",           CreateMaterial(V4(1.0,      1.0,      1.0,      1), V4( 1.0,      1.0,        1.0,        1), V4( 1.0,        1.0,        1.0,        1), 1.0,        false, BitmapIndex));
  PushMaterialData(AssetManager, "red",             CreateMaterial(V4(1.0,      0.0,      0.0,      1), V4( 1.0,      0.0,        0.0,        1), V4( 1.0,        0.0,        0.0,        1), 1.0,        false, BitmapIndex));
  PushMaterialData(AssetManager, "green",           CreateMaterial(V4(0.0,      1.0,      0.0,      1), V4( 0.0,      1.0,        0.0,        1), V4( 0.0,        1.0,        0.0,        1), 1.0,        false, BitmapIndex));
  PushMaterialData(AssetManager, "blue",            CreateMaterial(V4(0.0,      0.0,      1.0,      1), V4( 0.0,      0.0,        1.0,        1), V4( 0.0,        0.0,        1.0,        1), 1.0,        false, BitmapIndex));
  PushMaterialData(AssetManager, "emerald",         CreateMaterial(V4(0.0215,   0.1745,   0.0215,   1), V4( 0.07568,  0.61424,    0.07568,    1), V4( 0.633,      0.727811,   0.633,      1), 0.6,        false, BitmapIndex));
  PushMaterialData(AssetManager, "jade",            CreateMaterial(V4(0.135,    0.2225,   0.1575,   1), V4( 0.54,     0.89,       0.63,       1), V4( 0.316228,   0.316228,   0.316228,   1), 0.1,        false, BitmapIndex));
  PushMaterialData(AssetManager, "obsidian",        CreateMaterial(V4(0.05375,  0.05,     0.06625,  1), V4( 0.18275,  0.17,       0.22525,    1), V4( 0.332741,   0.328634,   0.3,        1), 0.3,        false, BitmapIndex));
  PushMaterialData(AssetManager, "pearl",           CreateMaterial(V4(0.25,     0.20725,  0.20725,  1), V4( 1.0,      0.829,      0.829,      1), V4( 0.296648,   0.296648,   0.088,      1), 0.088,      false, BitmapIndex));
  PushMaterialData(AssetManager, "ruby",            CreateMaterial(V4(0.1745,   0.01175,  0.01175,  1), V4( 0.61424,  0.04136,    0.04136,    1), V4( 0.727811,   0.626959,   0.6,        1), 0.6,        false, BitmapIndex));
  PushMaterialData(AssetManager, "turquoise",       CreateMaterial(V4(0.1,      0.18725,  0.1745,   1), V4( 0.396,    0.74151,    0.69102,    1), V4( 0.297254,   0.30829,    0.1,        1), 0.1,        false, BitmapIndex));
  PushMaterialData(AssetManager, "brass",           CreateMaterial(V4(0.329412, 0.223529, 0.027451, 1), V4( 0.780392, 0.568627,   0.113725,   1), V4( 0.992157,   0.941176,   0.21794872, 1), 0.21794872, false, BitmapIndex));
  PushMaterialData(AssetManager, "bronze",          CreateMaterial(V4(0.2125,   0.1275,   0.054,    1), V4( 0.714,    0.4284,     0.18144,    1), V4( 0.393548,   0.271906,   0.2,        1), 0.2,        false, BitmapIndex));
  PushMaterialData(AssetManager, "chrome",          CreateMaterial(V4(0.25,     0.25,     0.25,     1), V4( 0.4,      0.4,        0.4,        1), V4( 0.774597,   0.774597,   0.6,        1), 0.6,        false, BitmapIndex));
  PushMaterialData(AssetManager, "compper",         CreateMaterial(V4(0.19125,  0.0735,   0.0225,   1), V4( 0.7038,   0.27048,    0.0828,     1), V4( 0.256777,   0.137622,   0.1,        1), 0.1,        false, BitmapIndex));
  PushMaterialData(AssetManager, "gold",            CreateMaterial(V4(0.24725,  0.1995,   0.0745,   1), V4( 0.75164,  0.60648,    0.22648,    1), V4( 0.628281,   0.555802,   0.4,        1), 0.4,        false, BitmapIndex));
  PushMaterialData(AssetManager, "silver",          CreateMaterial(V4(0.19225,  0.19225,  0.19225,  1), V4( 0.50754,  0.50754,    0.50754,    1), V4( 0.508273,   0.508273,   0.4,        1), 0.4,        false, BitmapIndex));
  PushMaterialData(AssetManager, "black_plastic",   CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.01,     0.01,       0.01,       1), V4( 0.50,       0.50,       0.25,       1), 0.25,       false, BitmapIndex));
  PushMaterialData(AssetManager, "cyan_plastic",    CreateMaterial(V4(0.0,      0.1,      0.06,     1), V4( 0.0,      0.50980392, 0.50980392, 1), V4( 0.50196078, 0.50196078, 0.25,       1), 0.25,       false, BitmapIndex));
  PushMaterialData(AssetManager, "green_plastic",   CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.1,      0.35,       0.1,        1), V4( 0.45,       0.55,       0.25,       1), 0.25,       false, BitmapIndex));
  PushMaterialData(AssetManager, "red_plastic",     CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.5,      0.0,        0.0,        1), V4( 0.7,        0.6,        0.25,       1), 0.25,       false, BitmapIndex));
  PushMaterialData(AssetManager, "blue_plastic",    CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.0,      0.0,        0.5,        1), V4( 0.6,        0.6,        0.25,       1), 0.25,       false, BitmapIndex));
  PushMaterialData(AssetManager, "white_plastic",   CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.55,     0.55,       0.55,       1), V4( 0.70,       0.70,       0.25,       1), 0.25,       false, BitmapIndex));
  PushMaterialData(AssetManager, "yellow_plastic",  CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.5,      0.5,        0.0,        1), V4( 0.60,       0.60,       0.25,       1), 0.25,       false, BitmapIndex));
  PushMaterialData(AssetManager, "black_rubber",    CreateMaterial(V4(0.02,     0.02,     0.02,     1), V4( 0.01,     0.01,       0.01,       1), V4( 0.4,        0.4,        0.078125,   1), 0.078125,   false, BitmapIndex));
  PushMaterialData(AssetManager, "cyan_rubber",     CreateMaterial(V4(0.0,      0.05,     0.05,     1), V4( 0.4,      0.5,        0.5,        1), V4( 0.04,       0.7,        0.078125,   1), 0.078125,   false, BitmapIndex));
  PushMaterialData(AssetManager, "green_rubber",    CreateMaterial(V4(0.0,      0.05,     0.0,      1), V4( 0.4,      0.5,        0.4,        1), V4( 0.04,       0.7,        0.078125,   1), 0.078125,   false, BitmapIndex));
  PushMaterialData(AssetManager, "red_rubber",      CreateMaterial(V4(0.05,     0.0,      0.0,      1), V4( 0.5,      0.4,        0.4,        1), V4( 0.7,        0.04,       0.078125,   1), 0.078125,   false, BitmapIndex));
  PushMaterialData(AssetManager, "blue_rubber",     CreateMaterial(V4(0.00,     0.0,      0.05,     1), V4( 0.4,      0.4,        0.5,        1), V4( 0.04,       0.04,       0.078125,   1), 0.078125,   false, BitmapIndex));
  PushMaterialData(AssetManager, "white_rubber",    CreateMaterial(V4(0.05,     0.05,     0.05,     1), V4( 0.5,      0.5,        0.5,        1), V4( 0.7,        0.7,        0.078125,   1), 0.078125,   false, BitmapIndex));
  PushMaterialData(AssetManager, "yellow_rubber",   CreateMaterial(V4(0.05,     0.05,     0.0,      1), V4( 0.5,      0.5,        0.4,        1), V4( 0.7,        0.7,        0.078125,   1), 0.078125,   false, BitmapIndex));
}


void LoadPredefinedMeshes(game_asset_manager* AssetManager)
{
  {
    v3 v[] =
    {
      V3(-0.5, -0.5, 0),
      V3( 0.5, -0.5, 0),
      V3( 0.5,  0.5, 0),
      V3(-0.5,  0.5, 0)
    };
    v3 vn[] =
    {
      V3( 0, 0, 1),
    };
    v2 vt[] =
    {
      V2( 0, 0),
      V2( 1, 0),
      V2( 1, 1),
      V2( 0, 1)
    };
    u32 MeshIndex = PushMeshData(AssetManager,
      ArrayCount(v),  v,
      ArrayCount(vn), vn,
      ArrayCount(vt), vt);

    u32 vi[] = {0,1,2,0,2,3};
    u32 ti[] = {0,1,2,0,2,3};
    u32 ni[] = {0,0,0,0,0,0};
    aabb3f AABB = AABB3f(V3(-0.5, -0.5, 0), V3( 0.5,  0.5, 0));
    PushIndexData(AssetManager, "quad", MeshIndex, ArrayCount(vi), vi, ti, ni, AABB);
  }

  {
    v3 v[] =
    {
      V3(-0.5,-0.5, 0.5),
      V3( 0.5,-0.5, 0.5),
      V3(-0.5, 0.5, 0.5),
      V3( 0.5, 0.5, 0.5),
      V3(-0.5, 0.5,-0.5),
      V3( 0.5, 0.5,-0.5),
      V3(-0.5,-0.5,-0.5),
      V3( 0.5,-0.5,-0.5)
    };

    v3 vn[] =
    {
      V3( 0.0, 0.0, 1.0),
      V3( 0.0, 1.0, 0.0),
      V3( 0.0, 0.0,-1.0),
      V3( 0.0,-1.0, 0.0),
      V3( 1.0, 0.0, 0.0),
      V3(-1.0, 0.0, 0.0)
    };

    v2 vt[] =
    {
      V2( 0, 0),
      V2( 1, 0),
      V2( 1, 1),
      V2( 0, 1)
    };

    u32 MeshIndex = PushMeshData(AssetManager,
      ArrayCount(v),  v,
      ArrayCount(vn), vn,
      ArrayCount(vt), vt);

    u32 vi[] = {0,1,2,2,1,3,2,3,4,4,3,5,4,5,6,6,5,7,6,7,0,0,7,1,1,7,3,3,7,5,6,0,4,4,0,2};
    u32 ti[] = {0,1,3,3,1,2,0,1,3,3,1,2,0,1,3,3,1,2,0,1,3,3,1,2,0,1,3,3,1,2,0,1,3,3,1,2};
    u32 ni[] = {0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,5,5,5,5,5,5};
    aabb3f AABB = AABB3f(V3(-0.5, -0.5, -0.5), V3( 0.5, 0.5, 0.5));
    PushIndexData(AssetManager, "voxel", MeshIndex, ArrayCount(vi), vi, ti, ni, AABB);
  }

}

internal void stbtt_BakeFontBitmap(game_asset_manager* AssetManager)
{
  const s32 Ranges[] =
  {
      0x20, 0x80,       // BasicLatin
      0xA1, 0x100,      // ExtendedLatin
      0x16A0, 0x1700,   // Runic
      0x600, 0x700,     // Arabic
      0x370, 0x400,     // Greek
      0x2200, 0x2300    // Mathematical
  };

  stb_font_map* FontMap = &AssetManager->FontMap;
  FontMap->StartChar = Ranges[0];
  FontMap->NumChars = Ranges[1] - Ranges[0];
  FontMap->FontHeightPx = 24.f;
  FontMap->CharData = PushArray(&AssetManager->AssetArena, FontMap->NumChars, stbtt_bakedchar);

  thread_context Thread;
  debug_read_file_result TTFFile = Platform.DEBUGPlatformReadEntireFile(&Thread, "C:\\Windows\\Fonts\\arial.ttf");
  Assert(TTFFile.Contents);
  stbtt_GetScaledFontVMetrics((u8*) TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*) TTFFile.Contents, 0),
   FontMap->FontHeightPx, &FontMap->Ascent, &FontMap->Descent, &FontMap->LineGap);
  
  // Todo: Insetead of saving the 8bit font map to a 32 bit, write a custom shader for the 8bit 
  //       texture that can render it with colour.
  u32 BPP = 32;
  u32 Width = 1028;
  u32 Height = 1028;
    
  u8* Pixels8BPP = PushArray(GlobalGameState->TransientArena, Width * Height, u8);

  s32 ret = stbtt_BakeFontBitmap((u8*) TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*) TTFFile.Contents, 0),
                       FontMap->FontHeightPx,
                       Pixels8BPP, Width, Height,
                       FontMap->StartChar, FontMap->NumChars,
                       FontMap->CharData);
  Assert(ret>0);

  u32* BasePixel = PushArray(GlobalGameState->TransientArena, Width*Height, u32 );
  u32* Pixels = BasePixel;
  u8*  SrcPixel =  Pixels8BPP;
  u8*  EndSrcPixel = Pixels8BPP + Width * Height;
  while(SrcPixel != EndSrcPixel)
  {
    u8 Alpha = *SrcPixel;
    u8 Blue = *SrcPixel;
    u8 Green = *SrcPixel;
    u8 Red = *SrcPixel;
    u32 PixelData = (Blue << 0) | (Green << 8) | (Red << 16) | Alpha << 24;
    *Pixels++ = PixelData;
    SrcPixel++;
  }
  FontMap->BitmapIndex = PushBitmapData(AssetManager, "debug_font", Width, Height, BPP, BasePixel);

  Platform.DEBUGPlatformFreeFileMemory(&Thread, TTFFile.Contents);
}

internal void LoadCubeAsset(game_asset_manager* AssetManager)
{
  memory_arena* AssetArena = &AssetManager->AssetArena;
  obj_loaded_file* LoadedObjFile = ReadOBJFile( AssetArena, GlobalGameState->TransientArena,
   "..\\handmade\\data\\cube\\cube.obj");

  mesh_data* MeshData = LoadedObjFile->MeshData;

  u32 MeshIndex = PushMeshData(AssetManager,
      MeshData->nv,  MeshData->v,
      MeshData->nvn, MeshData->vn,
      MeshData->nvt, MeshData->vt);

  for (u32 i = 0; i < LoadedObjFile->ObjectCount; ++i)
  {
    obj_group* LoadedObjectGroup = LoadedObjFile->Objects + i;
    Assert(LoadedObjectGroup->GroupName);
    PushIndexData(AssetManager,
      LoadedObjectGroup->GroupName,
      MeshIndex,
      LoadedObjectGroup->Indeces->Count, 
      LoadedObjectGroup->Indeces->vi,
      LoadedObjectGroup->Indeces->ti,
      LoadedObjectGroup->Indeces->ni,
      LoadedObjectGroup->aabb);
  }

  for (u32 i = 0; i < LoadedObjFile->MaterialData->MaterialCount; ++i)
  {
    mtl_material* SrcMaterial = LoadedObjFile->MaterialData->Materials + i;
    Assert(SrcMaterial->NameLength > 0);
    Assert(SrcMaterial->NameLength < 120);
    Assert(SrcMaterial->Name);

    u32 BitmapHandle = 0;
    if(SrcMaterial->MapKd)
    {  
      c8 TexKey[128] = {};
      c8 Appendix[] = "_kd";
      str::CatStrings( SrcMaterial->NameLength, SrcMaterial->Name,
        ArrayCount(Appendix), Appendix, ArrayCount(TexKey),  TexKey );
      BitmapHandle = PushBitmapData(AssetManager, TexKey,
        SrcMaterial->MapKd->Width, SrcMaterial->MapKd->Height,
        SrcMaterial->MapKd->BPP,   SrcMaterial->MapKd->Pixels);
    }

    material Material = CreateMaterial(*SrcMaterial->Ka, *SrcMaterial->Kd, *SrcMaterial->Ks, *SrcMaterial->Ns, false, BitmapHandle);
    PushMaterialData(AssetManager, SrcMaterial->Name, Material);
  }
}

internal void LoadHeroSpriteSheet(game_asset_manager* AssetManager)
{
  bitmap* Bitmap = LoadTGA(&AssetManager->AssetArena,
        "..\\handmade\\data\\Platformer\\Adventurer\\adventurer-Sheet.tga" );
  u32 BitmapHandle = PushBitmapData(AssetManager, "hero_sprite_sheet", Bitmap->Width, Bitmap->Height, Bitmap->BPP, Bitmap->Pixels);
  PushMaterialData(AssetManager, "hero_sprite_sheet", CreateMaterial(V4(1,1,1,1),V4(1,1,1,1),V4(1,1,1,1),1,true, BitmapHandle));
}

void LoadAssets()
{
  game_asset_manager* AssetManager = GlobalGameState->AssetManager;
  LoadPredefinedMaterials(AssetManager);
  LoadPredefinedMeshes(AssetManager);
  stbtt_BakeFontBitmap(AssetManager);
  LoadCubeAsset(AssetManager);  
  LoadHeroSpriteSheet(AssetManager);
}

game_asset_manager* CreateAssetManager()
{
  game_asset_manager* AssetManager = BootstrapPushStruct(game_asset_manager, AssetArena);

  AssetManager->Instances.MaxCount = 256;
  AssetManager->Instances.Values = (void**) PushArray(&AssetManager->AssetArena,
                                            AssetManager->Instances.MaxCount, void*);
  AssetManager->TemporaryInstancesBase = 0;
  AssetManager->TemporaryInstancesCount = 0;

  AssetManager->Meshes.MaxCount = 64;
  AssetManager->Meshes.Values = (void**) PushArray(&AssetManager->AssetArena,
                                  AssetManager->Meshes.MaxCount, mesh_data);

  AssetManager->Objects.MaxCount = 64;
  AssetManager->Objects.Values = (void**) PushArray(&AssetManager->AssetArena,
                                            AssetManager->Objects.MaxCount, void*);
  AssetManager->ObjectKeeper = PushArray(&AssetManager->AssetArena,
                                            AssetManager->Objects.MaxCount, book_keeper);

  AssetManager->Materials.MaxCount = 64;
  AssetManager->Materials.Values = (void**) PushArray( &AssetManager->AssetArena,
                                            AssetManager->Materials.MaxCount, void*);

  AssetManager->Bitmaps.MaxCount = 64;
  AssetManager->Bitmaps.Values = (void**) PushArray( &AssetManager->AssetArena,
                                            AssetManager->Materials.MaxCount, void*);
  AssetManager->BitmapKeeper = PushArray(&AssetManager->AssetArena,
                                            AssetManager->Materials.MaxCount, book_keeper);
  return AssetManager;
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
      Result = GetNamedHandle(&AssetManager->Objects, Key);
    }break;
    case asset_type::MATERIAL:
    {
      Result = GetNamedHandle(&AssetManager->Materials, Key);
    }break;
    case asset_type::BITMAP:
    {
      Result = GetNamedHandle(&AssetManager->Bitmaps, Key);
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