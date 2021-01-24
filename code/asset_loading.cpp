#include "assets.h"

internal void LoadPredefinedMaterials(game_asset_manager* AssetManager)
{
  u8 WhitePixel[4] = {255,255,255,255};
  PushBitmapData(AssetManager, "null", 1, 1, 32, (void*)WhitePixel, true);

  //u32 WhiteTexture[256][256];
  //for (int i = 0; i < 256; ++i)
  //{
  //  for (int j = 0; j < 256; ++j)
  //  {
  //    u8* rgb = (u8*) &WhiteTexture[i][j];
  //    *rgb++ = 0xFF;
  //    *rgb++ = 0xFF;
  //    *rgb++ = 0xFF;
  //    *rgb++ = 0xFF;
  //  }
  //}
  //PushBitmapData(AssetManager, "null2", 256, 256, 32, (void*)WhiteTexture, false);

  PushMaterialData(AssetManager, "white",           CreateMaterial(V4(1.0,      1.0,      1.0,      1), V4( 1.0,      1.0,        1.0,        1), V4( 1.0,        1.0,        1.0,        1), 1.0,        false));
  PushMaterialData(AssetManager, "red",             CreateMaterial(V4(1.0,      0.0,      0.0,      1), V4( 1.0,      0.0,        0.0,        1), V4( 1.0,        0.0,        0.0,        1), 1.0,        false));
  PushMaterialData(AssetManager, "green",           CreateMaterial(V4(0.0,      1.0,      0.0,      1), V4( 0.0,      1.0,        0.0,        1), V4( 0.0,        1.0,        0.0,        1), 1.0,        false));
  PushMaterialData(AssetManager, "blue",            CreateMaterial(V4(0.0,      0.0,      1.0,      1), V4( 0.0,      0.0,        1.0,        1), V4( 0.0,        0.0,        1.0,        1), 1.0,        false));
  PushMaterialData(AssetManager, "emerald",         CreateMaterial(V4(0.0215,   0.1745,   0.0215,   1), V4( 0.07568,  0.61424,    0.07568,    1), V4( 0.633,      0.727811,   0.633,      1), 0.6,        false));
  PushMaterialData(AssetManager, "jade",            CreateMaterial(V4(0.135,    0.2225,   0.1575,   1), V4( 0.54,     0.89,       0.63,       1), V4( 0.316228,   0.316228,   0.316228,   1), 0.1,        false));
  PushMaterialData(AssetManager, "obsidian",        CreateMaterial(V4(0.05375,  0.05,     0.06625,  1), V4( 0.18275,  0.17,       0.22525,    1), V4( 0.332741,   0.328634,   0.3,        1), 0.3,        false));
  PushMaterialData(AssetManager, "pearl",           CreateMaterial(V4(0.25,     0.20725,  0.20725,  1), V4( 1.0,      0.829,      0.829,      1), V4( 0.296648,   0.296648,   0.088,      1), 0.088,      false));
  PushMaterialData(AssetManager, "ruby",            CreateMaterial(V4(0.1745,   0.01175,  0.01175,  1), V4( 0.61424,  0.04136,    0.04136,    1), V4( 0.727811,   0.626959,   0.6,        1), 0.6,        false));
  PushMaterialData(AssetManager, "turquoise",       CreateMaterial(V4(0.1,      0.18725,  0.1745,   1), V4( 0.396,    0.74151,    0.69102,    1), V4( 0.297254,   0.30829,    0.1,        1), 0.1,        false));
  PushMaterialData(AssetManager, "brass",           CreateMaterial(V4(0.329412, 0.223529, 0.027451, 1), V4( 0.780392, 0.568627,   0.113725,   1), V4( 0.992157,   0.941176,   0.21794872, 1), 0.21794872, false));
  PushMaterialData(AssetManager, "bronze",          CreateMaterial(V4(0.2125,   0.1275,   0.054,    1), V4( 0.714,    0.4284,     0.18144,    1), V4( 0.393548,   0.271906,   0.2,        1), 0.2,        false));
  PushMaterialData(AssetManager, "chrome",          CreateMaterial(V4(0.25,     0.25,     0.25,     1), V4( 0.4,      0.4,        0.4,        1), V4( 0.774597,   0.774597,   0.6,        1), 0.6,        false));
  PushMaterialData(AssetManager, "compper",         CreateMaterial(V4(0.19125,  0.0735,   0.0225,   1), V4( 0.7038,   0.27048,    0.0828,     1), V4( 0.256777,   0.137622,   0.1,        1), 0.1,        false));
  PushMaterialData(AssetManager, "gold",            CreateMaterial(V4(0.24725,  0.1995,   0.0745,   1), V4( 0.75164,  0.60648,    0.22648,    1), V4( 0.628281,   0.555802,   0.4,        1), 0.4,        false));
  PushMaterialData(AssetManager, "silver",          CreateMaterial(V4(0.19225,  0.19225,  0.19225,  1), V4( 0.50754,  0.50754,    0.50754,    1), V4( 0.508273,   0.508273,   0.4,        1), 0.4,        false));
  PushMaterialData(AssetManager, "black_plastic",   CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.01,     0.01,       0.01,       1), V4( 0.50,       0.50,       0.25,       1), 0.25,       false));
  PushMaterialData(AssetManager, "cyan_plastic",    CreateMaterial(V4(0.0,      0.1,      0.06,     1), V4( 0.0,      0.50980392, 0.50980392, 1), V4( 0.50196078, 0.50196078, 0.25,       1), 0.25,       false));
  PushMaterialData(AssetManager, "green_plastic",   CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.1,      0.35,       0.1,        1), V4( 0.45,       0.55,       0.25,       1), 0.25,       false));
  PushMaterialData(AssetManager, "red_plastic",     CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.5,      0.0,        0.0,        1), V4( 0.7,        0.6,        0.25,       1), 0.25,       false));
  PushMaterialData(AssetManager, "blue_plastic",    CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.0,      0.0,        0.5,        1), V4( 0.6,        0.6,        0.25,       1), 0.25,       false));
  PushMaterialData(AssetManager, "white_plastic",   CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.55,     0.55,       0.55,       1), V4( 0.70,       0.70,       0.25,       1), 0.25,       false));
  PushMaterialData(AssetManager, "yellow_plastic",  CreateMaterial(V4(0.0,      0.0,      0.0,      1), V4( 0.5,      0.5,        0.0,        1), V4( 0.60,       0.60,       0.25,       1), 0.25,       false));
  PushMaterialData(AssetManager, "black_rubber",    CreateMaterial(V4(0.02,     0.02,     0.02,     1), V4( 0.01,     0.01,       0.01,       1), V4( 0.4,        0.4,        0.078125,   1), 0.078125,   false));
  PushMaterialData(AssetManager, "cyan_rubber",     CreateMaterial(V4(0.0,      0.05,     0.05,     1), V4( 0.4,      0.5,        0.5,        1), V4( 0.04,       0.7,        0.078125,   1), 0.078125,   false));
  PushMaterialData(AssetManager, "green_rubber",    CreateMaterial(V4(0.0,      0.05,     0.0,      1), V4( 0.4,      0.5,        0.4,        1), V4( 0.04,       0.7,        0.078125,   1), 0.078125,   false));
  PushMaterialData(AssetManager, "red_rubber",      CreateMaterial(V4(0.05,     0.0,      0.0,      1), V4( 0.5,      0.4,        0.4,        1), V4( 0.7,        0.04,       0.078125,   1), 0.078125,   false));
  PushMaterialData(AssetManager, "blue_rubber",     CreateMaterial(V4(0.00,     0.0,      0.05,     1), V4( 0.4,      0.4,        0.5,        1), V4( 0.04,       0.04,       0.078125,   1), 0.078125,   false));
  PushMaterialData(AssetManager, "white_rubber",    CreateMaterial(V4(0.05,     0.05,     0.05,     1), V4( 0.5,      0.5,        0.5,        1), V4( 0.7,        0.7,        0.078125,   1), 0.078125,   false));
  PushMaterialData(AssetManager, "yellow_rubber",   CreateMaterial(V4(0.05,     0.05,     0.0,      1), V4( 0.5,      0.5,        0.4,        1), V4( 0.7,        0.7,        0.078125,   1), 0.078125,   false));
}

internal void LoadPredefinedMeshes(game_asset_manager* AssetManager)
{
  {
    v3 v[] =
    {
      V3(-0.5,-0.5, 0),
      V3( 0.5,-0.5, 0),
      V3(-0.5, 0.5, 0),
      V3( 0.5, 0.5, 0),
    };
    v3 vn[] =
    {
      V3( 0, 0, 1)
    };
    v2 vt[] =
    {
      V2( 0, 0),
      V2( 1, 0),
      V2( 0, 1),
      V2( 1, 1)
    };
    mesh_handle MeshHandle = PushMeshData(AssetManager,
      ArrayCount(v),  v,
      ArrayCount(vn), vn,
      ArrayCount(vt), vt);

    u32 vi[] = {0,1,2,2,1,3};
    u32 ti[] = {0,1,2,2,1,3};
    u32 ni[] = {0,0,0,0,0,0};
    aabb3f AABB = AABB3f(V3(-0.5, -0.5, 0), V3( 0.5,  0.5, 0));
    object_handle ObjectHandle = PushIndexData(AssetManager, "quad", MeshHandle, ArrayCount(vi), vi, ti, ni, AABB);
    AssetManager->EnumeratedMeshes[(u32)predefined_mesh::QUAD] = ObjectHandle;
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

    mesh_handle MeshHandle = PushMeshData(AssetManager,
      ArrayCount(v),  v,
      ArrayCount(vn), vn,
      ArrayCount(vt), vt);

    u32 vi[] = {0,1,2,2,1,3,2,3,4,4,3,5,4,5,6,6,5,7,6,7,0,0,7,1,1,7,3,3,7,5,6,0,4,4,0,2};
    u32 ti[] = {0,1,3,3,1,2,0,1,3,3,1,2,0,1,3,3,1,2,0,1,3,3,1,2,0,1,3,3,1,2,0,1,3,3,1,2};
    u32 ni[] = {0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,5,5,5,5,5,5};
    aabb3f AABB = AABB3f(V3(-0.5, -0.5, -0.5), V3( 0.5, 0.5, 0.5));
    object_handle ObjectHandle = PushIndexData(AssetManager, "voxel", MeshHandle, ArrayCount(vi), vi, ti, ni, AABB);
    AssetManager->EnumeratedMeshes[(u32)predefined_mesh::VOXEL] = ObjectHandle;
  }


  object_handle Handle;
  GetHandle(AssetManager, "voxel", &Handle);
  GetHandle(AssetManager, "quad", &Handle);

}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "externals/stb_image_write.h"


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


  thread_context Thread;
  debug_read_file_result TTFFile = Platform.DEBUGPlatformReadEntireFile(&Thread, "C:\\Users\\jh\\Documents\\dev\\handmade\\data\\Fonts\\Mx437_IBM_BIOS.ttf");
  Assert(TTFFile.Contents);

  auto push_font = [&Ranges, &AssetManager, &TTFFile](stb_font_map* FontMap, c8* Name, r32 FontHeightPx)
  {
    FontMap->StartChar = Ranges[0];
    FontMap->NumChars = Ranges[1] - Ranges[0];
    FontMap->FontHeightPx = FontHeightPx;
    FontMap->CharData = PushArray(&AssetManager->AssetArena, FontMap->NumChars, stbtt_bakedchar);

    stbtt_GetScaledFontVMetrics((u8*) TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*) TTFFile.Contents, 0),
     FontMap->FontHeightPx, &FontMap->Ascent, &FontMap->Descent, &FontMap->LineGap);

    // Todo: Insetead of saving the 8bit font map to a 32 bit, write a custom shader for the 8bit
    //       texture that can render it with colour.

    // Todo: Character clipping seems to be dependent on 512x512 texture size. Try to make it dynamic
    //       so that any texturesize works
    u32 BPP = 32;
    u32 Width = 512;
    u32 Height = 512;

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

    PushBitmapData(AssetManager, Name, Width, Height, BPP, BasePixel, false);
    GetHandle(AssetManager, Name, &FontMap->BitmapHandle);
  };


  push_font(&AssetManager->FontMap[0], "debug_font_12", 8);
  push_font(&AssetManager->FontMap[1], "debug_font_14", 12);
  push_font(&AssetManager->FontMap[2], "debug_font_16", 16);
  push_font(&AssetManager->FontMap[3], "debug_font_18", 18);
  push_font(&AssetManager->FontMap[4], "debug_font_20", 20);
  push_font(&AssetManager->FontMap[5], "debug_font_22", 22);
  push_font(&AssetManager->FontMap[6], "debug_font_24", 24);
  push_font(&AssetManager->FontMap[7], "debug_font_26", 26);

  Platform.DEBUGPlatformFreeFileMemory(&Thread, TTFFile.Contents);
}

internal void LoadCubeAsset(game_asset_manager* AssetManager)
{
  memory_arena* AssetArena = &AssetManager->AssetArena;
  obj_loaded_file* LoadedObjFile = ReadOBJFile( AssetArena, GlobalGameState->TransientArena,
   "..\\handmade\\data\\cube\\cube.obj");

  mesh_data* MeshData = LoadedObjFile->MeshData;

  mesh_handle MeshHandle = PushMeshData(AssetManager,
      MeshData->nv,  MeshData->v,
      MeshData->nvn, MeshData->vn,
      MeshData->nvt, MeshData->vt);

  for (u32 i = 0; i < LoadedObjFile->ObjectCount; ++i)
  { 
    obj_group* LoadedObjectGroup = LoadedObjFile->Objects + i;
    Assert(LoadedObjectGroup->GroupName);
    PushIndexData(AssetManager,
      LoadedObjectGroup->GroupName,
      MeshHandle,
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

    bitmap_handle BitmapHandle = {};
    if(SrcMaterial->MapKd)
    {
      c8 TexKey[128] = {};
      c8 Appendix[] = "_kd";
      str::CatStrings( SrcMaterial->NameLength, SrcMaterial->Name,
        ArrayCount(Appendix), Appendix, ArrayCount(TexKey),  TexKey );
      BitmapHandle = PushBitmapData(AssetManager, TexKey,
        SrcMaterial->MapKd->Width, SrcMaterial->MapKd->Height,
        SrcMaterial->MapKd->BPP,   SrcMaterial->MapKd->Pixels, true);
    }

    material Material = CreateMaterial(*SrcMaterial->Ka, *SrcMaterial->Kd, *SrcMaterial->Ks, *SrcMaterial->Ns, false);
    PushMaterialData(AssetManager, SrcMaterial->Name, Material);
  }
}

internal void LoadBitmaps(game_asset_manager* AssetManager)
{
  bitmap* HeroBitmap = LoadTGA(&AssetManager->AssetArena,
        "..\\handmade\\data\\Platformer\\Adventurer\\adventurer-Sheet.tga" );
  bitmap_handle HeroBitmapHandle = PushBitmapData(AssetManager, "hero_sprite_sheet", HeroBitmap->Width, HeroBitmap->Height, HeroBitmap->BPP, HeroBitmap->Pixels, true);
  PushMaterialData(AssetManager, "hero_sprite_sheet", CreateMaterial(V4(1,1,1,1),V4(1,1,1,1),V4(1,1,1,1),1,true));

  bitmap* CheckerBitmap = LoadTGA(&AssetManager->AssetArena,
        "..\\handmade\\data\\checker_board.tga" );
  bitmap_handle CheckerBitmapHandle = PushBitmapData(AssetManager, "checker_board", CheckerBitmap->Width, CheckerBitmap->Height, CheckerBitmap->BPP, CheckerBitmap->Pixels, true);
  PushMaterialData(AssetManager, "checker_board", CreateMaterial(V4(1,1,1,1),V4(1,1,1,1),V4(1,1,1,1),1,true));
}

internal void LoadTeapotAsset(game_asset_manager* AssetManager)
{
  memory_arena* AssetArena = &AssetManager->AssetArena;
  obj_loaded_file* LoadedObjFile = ReadOBJFile( AssetArena, GlobalGameState->TransientArena,
   "..\\handmade\\data\\teapot.obj");

  mesh_data* MeshData = LoadedObjFile->MeshData;

  mesh_handle MeshHandle = PushMeshData(AssetManager,
      MeshData->nv,  MeshData->v,
      MeshData->nvn, MeshData->vn,
      MeshData->nvt, MeshData->vt);

  for (u32 i = 0; i < LoadedObjFile->ObjectCount; ++i)
  {
    obj_group* LoadedObjectGroup = LoadedObjFile->Objects + i;
    Assert(LoadedObjectGroup->GroupName);
    PushIndexData(AssetManager,
      LoadedObjectGroup->GroupName,
      MeshHandle,
      LoadedObjectGroup->Indeces->Count,
      LoadedObjectGroup->Indeces->vi,
      LoadedObjectGroup->Indeces->ti,
      LoadedObjectGroup->Indeces->ni,
      LoadedObjectGroup->aabb);
  }
}

internal void LoadAssets(game_asset_manager* AssetManager)
{
  LoadPredefinedMaterials(AssetManager);
  LoadPredefinedMeshes(AssetManager);
  stbtt_BakeFontBitmap(AssetManager);
  LoadCubeAsset(AssetManager);
  LoadBitmaps(AssetManager);
  LoadTeapotAsset(AssetManager);
}

game_asset_manager* CreateAssetManager()
{
  game_asset_manager* AssetManager = BootstrapPushStruct(game_asset_manager, AssetArena);

  AssetManager->Meshes.MaxCount = 64;
  AssetManager->Meshes.Values = (void**) PushArray(&AssetManager->AssetArena,
                                  AssetManager->Meshes.MaxCount, mesh_data);

  AssetManager->Objects.MaxCount = 64;
  AssetManager->Objects.Values = (void**) PushArray(&AssetManager->AssetArena,
                                            AssetManager->Objects.MaxCount, void*);
  AssetManager->ObjectKeeper = PushArray(&AssetManager->AssetArena,
                                            AssetManager->Objects.MaxCount, buffer_keeper);

  AssetManager->Materials.MaxCount = 64;
  AssetManager->Materials.Values = (void**) PushArray( &AssetManager->AssetArena,
                                            AssetManager->Materials.MaxCount, void*);
  AssetManager->Bitmaps.MaxCount = 64;
  AssetManager->Bitmaps.Values = (void**) PushArray( &AssetManager->AssetArena,
                                            AssetManager->Materials.MaxCount, void*);
  AssetManager->BitmapKeeper = PushArray(&AssetManager->AssetArena,
                                            AssetManager->Materials.MaxCount, bitmap_keeper);

  AssetManager->EnumeratedMeshes = PushArray(&AssetManager->AssetArena,
                                            (u32)predefined_mesh::COUNT, object_handle);

  AssetManager->ObjectPendingLoadCount = 0;
  AssetManager->BitmapPendingLoadCount = 0;

  LoadAssets(AssetManager);

  return AssetManager;
}