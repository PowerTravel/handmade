#pragma once

#include "bitmap.h"
#include "data_containers.h"
#include "math/affine_transformations.h"

hash_map<bitmap_coordinate> LoadTileSpriteSheetCoordinates(memory_arena* Arena)
{
  hash_map<bitmap_coordinate> Result(Arena, 172);
  Result.Insert( "box", { 0, 864, 70, 70 });
  Result.Insert( "boxAlt", { 0, 792, 70, 70 });
  Result.Insert( "boxCoin", { 0, 720, 70, 70 });
  Result.Insert( "boxCoinAlt", { 0, 576, 70, 70 });
  Result.Insert( "boxCoinAlt_disabled", { 0, 504, 70, 70 });
  Result.Insert( "boxCoin_disabled", { 0, 648, 70, 70 });
  Result.Insert( "boxEmpty", { 0, 432, 70, 70 });
  Result.Insert( "boxExplosive", { 0, 360, 70, 70 });
  Result.Insert( "boxExplosiveAlt", { 0, 216, 70, 70 });
  Result.Insert( "boxExplosive_disabled", { 0, 288, 70, 70 });
  Result.Insert( "boxItem", { 0, 144, 70, 70 });
  Result.Insert( "boxItemAlt", { 0, 0, 70, 70 });
  Result.Insert( "boxItemAlt_disabled", { 432, 432, 70, 70 });
  Result.Insert( "boxItem_disabled", { 0, 72, 70, 70 });
  Result.Insert( "boxWarning", { 72, 648, 70, 70 });
  Result.Insert( "brickWall", { 216, 0, 70, 70 });
  Result.Insert( "bridge", { 216, 72, 70, 70 });
  Result.Insert( "bridgeLogs", { 288, 720, 70, 70 });
  Result.Insert( "castle", { 288, 792, 70, 70 });
  Result.Insert( "castleCenter", { 504, 288, 70, 70 });
  Result.Insert( "castleCenter_rounded", { 504, 720, 70, 70 });
  Result.Insert( "castleCliffLeft", { 504, 792, 70, 70 });
  Result.Insert( "castleCliffLeftAlt", { 648, 720, 70, 70 });
  Result.Insert( "castleCliffRight", { 648, 792, 70, 70 });
  Result.Insert( "castleCliffRightAlt", { 792, 288, 70, 70 });
  Result.Insert( "castleHalf", { 792, 360, 70, 70 });
  Result.Insert( "castleHalfLeft", { 432, 720, 70, 70 });
  Result.Insert( "castleHalfMid", { 648, 648, 70, 70 });
  Result.Insert( "castleHalfRight", { 792, 648, 70, 70 });
  Result.Insert( "castleHillLeft", { 648, 576, 70, 70 });
  Result.Insert( "castleHillLeft2", { 792, 576, 70, 70 });
  Result.Insert( "castleHillRight", { 792, 504, 70, 70 });
  Result.Insert( "castleHillRight2", { 792, 432, 70, 70 });
  Result.Insert( "castleLedgeLeft", { 856, 868, 5, 22 });
  Result.Insert( "castleLedgeRight", { 842, 868, 5, 22 });
  Result.Insert( "castleLeft", { 792, 216, 70, 70 });
  Result.Insert( "castleMid", { 792, 144, 70, 70 });
  Result.Insert( "castleRight", { 792, 72, 70, 70 });
  Result.Insert( "dirt", { 792, 0, 70, 70 });
  Result.Insert( "dirtCenter", { 720, 864, 70, 70 });
  Result.Insert( "dirtCenter_rounded", { 720, 792, 70, 70 });
  Result.Insert( "dirtCliffLeft", { 720, 720, 70, 70 });
  Result.Insert( "dirtCliffLeftAlt", { 720, 648, 70, 70 });
  Result.Insert( "dirtCliffRight", { 720, 576, 70, 70 });
  Result.Insert( "dirtCliffRightAlt", { 720, 504, 70, 70 });
  Result.Insert( "dirtHalf", { 720, 432, 70, 70 });
  Result.Insert( "dirtHalfLeft", { 720, 360, 70, 70 });
  Result.Insert( "dirtHalfMid", { 720, 288, 70, 70 });
  Result.Insert( "dirtHalfRight", { 720, 216, 70, 70 });
  Result.Insert( "dirtHillLeft", { 720, 144, 70, 70 });
  Result.Insert( "dirtHillLeft2", { 720, 72, 70, 70 });
  Result.Insert( "dirtHillRight", { 720, 0, 70, 70 });
  Result.Insert( "dirtHillRight2", { 648, 864, 70, 70 });
  Result.Insert( "dirtLedgeLeft", { 842, 892, 5, 18 });
  Result.Insert( "dirtLedgeRight", { 842, 912, 5, 18 });
  Result.Insert( "dirtLeft", { 504, 432, 70, 70 });
  Result.Insert( "dirtMid", { 504, 360, 70, 70 });
  Result.Insert( "dirtRight", { 648, 504, 70, 70 });
  Result.Insert( "door_closedMid", { 648, 432, 70, 70 });
  Result.Insert( "door_closedTop", { 648, 360, 70, 70 });
  Result.Insert( "door_openMid", { 648, 288, 70, 70 });
  Result.Insert( "door_openTop", { 648, 216, 70, 70 });
  Result.Insert( "fence", { 648, 144, 70, 70 });
  Result.Insert( "fenceBroken", { 648, 72, 70, 70 });
  Result.Insert( "grass", { 648, 0, 70, 70 });
  Result.Insert( "grassCenter", { 576, 864, 70, 70 });
  Result.Insert( "grassCenter_rounded", { 576, 792, 70, 70 });
  Result.Insert( "grassCliffLeft", { 576, 720, 70, 70 });
  Result.Insert( "grassCliffLeftAlt", { 576, 648, 70, 70 });
  Result.Insert( "grassCliffRight", { 576, 576, 70, 70 });
  Result.Insert( "grassCliffRightAlt", { 576, 504, 70, 70 });
  Result.Insert( "grassHalf", { 576, 432, 70, 70 });
  Result.Insert( "grassHalfLeft", { 576, 360, 70, 70 });
  Result.Insert( "grassHalfMid", { 576, 288, 70, 70 });
  Result.Insert( "grassHalfRight", { 576, 216, 70, 70 });
  Result.Insert( "grassHillLeft", { 576, 144, 70, 70 });
  Result.Insert( "grassHillLeft2", { 576, 72, 70, 70 });
  Result.Insert( "grassHillRight", { 576, 0, 70, 70 });
  Result.Insert( "grassHillRight2", { 504, 864, 70, 70 });
  Result.Insert( "grassLedgeLeft", { 849, 868, 5, 24 });
  Result.Insert( "grassLedgeRight", { 849, 894, 5, 24 });
  Result.Insert( "grassLeft", { 504, 648, 70, 70 });
  Result.Insert( "grassMid", { 504, 576, 70, 70 });
  Result.Insert( "grassRight", { 504, 504, 70, 70 });
  Result.Insert( "hill_large", { 842, 720, 48, 146 });
  Result.Insert( "hill_largeAlt", { 864, 0, 48, 146 });
  Result.Insert( "hill_small", { 792, 828, 48, 106 });
  Result.Insert( "hill_smallAlt", { 792, 720, 48, 106 });
  Result.Insert( "ladder_mid", { 504, 144, 70, 70 });
  Result.Insert( "ladder_top", { 504, 72, 70, 70 });
  Result.Insert( "liquidLava", { 504, 0, 70, 70 });
  Result.Insert( "liquidLavaTop", { 432, 864, 70, 70 });
  Result.Insert( "liquidLavaTop_mid", { 432, 792, 70, 70 });
  Result.Insert( "liquidWater", { 504, 216, 70, 70 });
  Result.Insert( "liquidWaterTop", { 432, 648, 70, 70 });
  Result.Insert( "liquidWaterTop_mid", { 432, 576, 70, 70 });
  Result.Insert( "lock_blue", { 432, 504, 70, 70 });
  Result.Insert( "lock_green", { 72, 576, 70, 70 });
  Result.Insert( "lock_red", { 432, 360, 70, 70 });
  Result.Insert( "lock_yellow", { 432, 288, 70, 70 });
  Result.Insert( "rockHillLeft", { 432, 216, 70, 70 });
  Result.Insert( "rockHillRight", { 432, 144, 70, 70 });
  Result.Insert( "ropeAttached", { 432, 72, 70, 70 });
  Result.Insert( "ropeHorizontal", { 432, 0, 70, 70 });
  Result.Insert( "ropeVertical", { 360, 864, 70, 70 });
  Result.Insert( "sand", { 360, 792, 70, 70 });
  Result.Insert( "sandCenter", { 576, 864, 70, 70 });
  Result.Insert( "sandCenter_rounded", { 576, 792, 70, 70 });
  Result.Insert( "sandCliffLeft", { 360, 720, 70, 70 });
  Result.Insert( "sandCliffLeftAlt", { 360, 648, 70, 70 });
  Result.Insert( "sandCliffRight", { 360, 576, 70, 70 });
  Result.Insert( "sandCliffRightAlt", { 360, 504, 70, 70 });
  Result.Insert( "sandHalf", { 360, 432, 70, 70 });
  Result.Insert( "sandHalfLeft", { 360, 360, 70, 70 });
  Result.Insert( "sandHalfMid", { 360, 288, 70, 70 });
  Result.Insert( "sandHalfRight", { 360, 216, 70, 70 });
  Result.Insert( "sandHillLeft", { 360, 144, 70, 70 });
  Result.Insert( "sandHillLeft2", { 360, 72, 70, 70 });
  Result.Insert( "sandHillRight", { 360, 0, 70, 70 });
  Result.Insert( "sandHillRight2", { 288, 864, 70, 70 });
  Result.Insert( "sandLedgeLeft", { 856, 892, 5, 18 });
  Result.Insert( "sandLedgeRight", { 856, 912, 5, 18 });
  Result.Insert( "sandLeft", { 288, 648, 70, 70 });
  Result.Insert( "sandMid", { 288, 576, 70, 70 });
  Result.Insert( "sandRight", { 288, 504, 70, 70 });
  Result.Insert( "sign", { 288, 432, 70, 70 });
  Result.Insert( "signExit", { 288, 360, 70, 70 });
  Result.Insert( "signLeft", { 288, 288, 70, 70 });
  Result.Insert( "signRight", { 288, 216, 70, 70 });
  Result.Insert( "snow", { 288, 144, 70, 70 });
  Result.Insert( "snowCenter", { 720, 864, 70, 70 });
  Result.Insert( "snowCenter_rounded", { 288, 72, 70, 70 });
  Result.Insert( "snowCliffLeft", { 288, 0, 70, 70 });
  Result.Insert( "snowCliffLeftAlt", { 216, 864, 70, 70 });
  Result.Insert( "snowCliffRight", { 216, 792, 70, 70 });
  Result.Insert( "snowCliffRightAlt", { 216, 720, 70, 70 });
  Result.Insert( "snowHalf", { 216, 648, 70, 70 });
  Result.Insert( "snowHalfLeft", { 216, 576, 70, 70 });
  Result.Insert( "snowHalfMid", { 216, 504, 70, 70 });
  Result.Insert( "snowHalfRight", { 216, 432, 70, 70 });
  Result.Insert( "snowHillLeft", { 216, 360, 70, 70 });
  Result.Insert( "snowHillLeft2", { 216, 288, 70, 70 });
  Result.Insert( "snowHillRight", { 216, 216, 70, 70 });
  Result.Insert( "snowHillRight2", { 216, 144, 70, 70 });
  Result.Insert( "snowLedgeLeft", { 863, 868, 5, 18 });
  Result.Insert( "snowLedgeRight", { 863, 888, 5, 18 });
  Result.Insert( "snowLeft", { 144, 864, 70, 70 });
  Result.Insert( "snowMid", { 144, 792, 70, 70 });
  Result.Insert( "snowRight", { 144, 720, 70, 70 });
  Result.Insert( "stone", { 144, 648, 70, 70 });
  Result.Insert( "stoneCenter", { 144, 576, 70, 70 });
  Result.Insert( "stoneCenter_rounded", { 144, 504, 70, 70 });
  Result.Insert( "stoneCliffLeft", { 144, 432, 70, 70 });
  Result.Insert( "stoneCliffLeftAlt", { 144, 360, 70, 70 });
  Result.Insert( "stoneCliffRight", { 144, 288, 70, 70 });
  Result.Insert( "stoneCliffRightAlt", { 144, 216, 70, 70 });
  Result.Insert( "stoneHalf", { 144, 144, 70, 70 });
  Result.Insert( "stoneHalfLeft", { 144, 72, 70, 70 });
  Result.Insert( "stoneHalfMid", { 144, 0, 70, 70 });
  Result.Insert( "stoneHalfRight", { 72, 864, 70, 70 });
  Result.Insert( "stoneHillLeft2", { 72, 792, 70, 70 });
  Result.Insert( "stoneHillRight2", { 72, 720, 70, 70 });
  Result.Insert( "stoneLedgeLeft", { 863, 908, 5, 24 });
  Result.Insert( "stoneLedgeRight", { 864, 148, 5, 24 });
  Result.Insert( "stoneLeft", { 72, 504, 70, 70 });
  Result.Insert( "stoneMid", { 72, 432, 70, 70 });
  Result.Insert( "stoneRight", { 72, 360, 70, 70 });
  Result.Insert( "stoneWall", { 72, 288, 70, 70 });
  Result.Insert( "tochLit", { 72, 216, 70, 70 });
  Result.Insert( "tochLit2", { 72, 144, 70, 70 });
  Result.Insert( "torch", { 72, 72, 70, 70 });
  Result.Insert( "window", { 72, 0, 70, 70 });
  return Result;
};

hash_map<bitmap_coordinate> LoadAdventurerSpriteSheetCoordinates(memory_arena* Arena)
{
  const u32 Width  = 50;
  const u32 Height = 37;
  hash_map<bitmap_coordinate> Result(Arena, 16);
  Result.Insert( "idle2_01",   {3 * Width,   5 * Height, Width, Height});
  Result.Insert( "idle2_02",   {4 * Width,   5 * Height, Width, Height});
  Result.Insert( "idle2_03",   {5 * Width,   5 * Height, Width, Height});
  Result.Insert( "idle2_04",   {6 * Width,   5 * Height, Width, Height});

  Result.Insert( "idle1_01",   {0 * Width,   0 * Height, Width, Height});
  Result.Insert( "idle1_02",   {1 * Width,   0 * Height, Width, Height});
  Result.Insert( "idle1_03",   {2 * Width,   0 * Height, Width, Height});
  Result.Insert( "idle1_04",   {3 * Width,   0 * Height, Width, Height});

  Result.Insert( "run_01",     {1 * Width,   1 * Height, Width, Height});
  Result.Insert( "run_02",     {2 * Width,   1 * Height, Width, Height});
  Result.Insert( "run_03",     {3 * Width,   1 * Height, Width, Height});
  Result.Insert( "run_04",     {4 * Width,   1 * Height, Width, Height});
  Result.Insert( "run_05",     {5 * Width,   1 * Height, Width, Height});
  Result.Insert( "run_06",     {6 * Width,   1 * Height, Width, Height});

  Result.Insert( "jump_01",     {6 * Width,   9 * Height, Width, Height});
  Result.Insert( "jump_02",     {0 * Width,   10 * Height, Width, Height});
  Result.Insert( "jump_03",     {1 * Width,   10 * Height, Width, Height});

  Result.Insert( "fall_01",     {1 * Width,   3 * Height, Width, Height});
  Result.Insert( "fall_02",     {2 * Width,   3 * Height, Width, Height});

  return Result;
};


m4 GetSpriteSheetTranslationMatrix(bitmap* SpriteSheet, bitmap_coordinate* Coordinate)
{
  r32 XMin = (r32) Coordinate->x;
  r32 Xoffset = XMin / (r32) SpriteSheet->Width;
  r32 ScaleX  =  (r32) Coordinate->w / (r32) SpriteSheet->Width;

  // Note: Picture is stored and read from bottom left to up and right but
  //     The coordinates given were top left to bottom right so we need to
  //     Invert the Y-Axis;
  r32 YMin = (r32) SpriteSheet->Height - (Coordinate->y + Coordinate->h);
  r32 Yoffset = YMin / (r32) SpriteSheet->Height;
  r32 ScaleY  =  (r32) Coordinate->h / (r32) SpriteSheet->Height;
  m4 Result = GetTranslationMatrix(V4(Xoffset,Yoffset,0,1)) * GetScaleMatrix(V4(ScaleX,ScaleY,0,1));
  return Result;
}