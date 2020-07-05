#pragma once

#include "types.h"

enum ColorDecodeMask
{
  BITMAP_MASK_ALPHA  = 0xff000000,
  BITMAP_MASK_BLUE   = 0xff0000,
  BITMAP_MASK_GREEN  = 0xff00,
  BITMAP_MASK_RED    = 0xff,
  BITMAP_SHIFT_ALPHA = 24,
  BITMAP_SHIFT_BLUE  = 16,
  BITMAP_SHIFT_GREEN = 8,
  BITMAP_SHIFT_RED   = 0
};

struct bitmap
{
  b32   Special;
  u32   BPP;
  u32   Width;
  u32   Height;
  void* Pixels;
};

bool GetPixelValue(bitmap* BitMap, const u32 X,  const u32 Y, u32* Result)
{
  if((X <= BitMap->Width) && (Y <= BitMap->Height))
  {
    *Result = *(( (u32*) BitMap->Pixels ) + BitMap->Width * Y + X);
    return true;
  }
  return false;
}

struct bitmap_coordinate
{
  u32 x;
  u32 y;
  u32 w;
  u32 h;
};

struct sprite_sheet
{
  bitmap* bitmap;
  u32 EntryCount;
};

void SplitPixelIntoARGBComponents(u32 PixelValue, u8* A, u8* R, u8* G, u8* B)
{
  *A = (u8) ((PixelValue & BITMAP_MASK_ALPHA) >> BITMAP_SHIFT_ALPHA);
  *R = (u8) ((PixelValue & BITMAP_MASK_RED)   >> BITMAP_SHIFT_RED);
  *G = (u8) ((PixelValue & BITMAP_MASK_GREEN) >> BITMAP_SHIFT_GREEN);
  *B = (u8) ((PixelValue & BITMAP_MASK_BLUE)  >> BITMAP_SHIFT_BLUE);
}