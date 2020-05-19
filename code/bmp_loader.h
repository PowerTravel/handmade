#pragma once

#include "memory.h"
#include "bitmap.h"
#include "platform.h"

// makes the packing compact
#pragma pack(push, 1)
struct bmp_header
{
  u16 FileType;     /* File type, always 4D42h ("BM") */
  u32 FileSize;     /* Size of the file in bytes */
  u16 Reserved1;    /* Always 0 */
  u16 Reserved2;    /* Always 0 */
  u32 BitmapOffset; /* Starting position of image data in bytes */

  u32 HeaderSize;       /* Size of this header in bytes */
  s32 Width;           /* Image width in pixels */
  s32 Height;          /* Image height in pixels */
  u16 Planes;          /* Number of color planes */
  u16 BitsPerPixel;    /* Number of bits per pixel */
  u32 Compression;     /* Compression methods used */
  u32 SizeOfBitmap;    /* Size of bitmap in bytes */
  s32 HorzResolution;  /* Horizontal resolution in pixels per meter */
  s32 VertResolution;  /* Vertical resolution in pixels per meter */
  u32 ColorsUsed;      /* Number of colors in the image */
  u32 ColorsImportant; /* Minimum number of important colors */
  /* Fields added for Windows 4.x follow this line */

  u32 RedMask;       /* Mask identifying bits of red component */
  u32 GreenMask;     /* Mask identifying bits of green component */
  u32 BlueMask;      /* Mask identifying bits of blue component */
  u32 AlphaMask;     /* Mask identifying bits of alpha component */
  u32 CSType;        /* Color space type */
  s32 RedX;          /* X coordinate of red endpoint */
  s32 RedY;          /* Y coordinate of red endpoint */
  s32 RedZ;          /* Z coordinate of red endpoint */
  s32 GreenX;        /* X coordinate of green endpoint */
  s32 GreenY;        /* Y coordinate of green endpoint */
  s32 GreenZ;        /* Z coordinate of green endpoint */
  s32 BlueX;         /* X coordinate of blue endpoint */
  s32 BlueY;         /* Y coordinate of blue endpoint */
  s32 BlueZ;         /* Z coordinate of blue endpoint */
  u32 GammaRed;      /* Gamma red coordinate scale value */
  u32 GammaGreen;    /* Gamma green coordinate scale value */
  u32 GammaBlue;     /* Gamma blue coordinate scale value */
};

#pragma pack(pop)
internal bitmap
DEBUGReadBMP(thread_context* Thread, game_state* aGameState,
  debug_platform_read_entire_file* ReadEntireFile,
  debug_platfrom_free_file_memory* FreeEntireFile,
  char* FileName)
{
  // Note(Jakob). BMP is stored in little Endian,
  // Little endian means that the least significant digit is stored first
  // So if we read a 32 bit int in 8 bit cunks it would come out as this:
  // 32bit: 0xAABBCCDD ->
  // 8bit chunks:
  // 0xDD, 0xCC, 0xBB, 0xAA

  // Todo(Jakob): Add hotspots to bitmaps. A hotspot is the alignpoint in pixels.
  //        For a mouse pointer sprite this would for example be on the tip of
  //        the pointer. Its used to poroperly display the bitmap.

  bitmap Result = {};
  debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

  if (ReadResult.ContentSize != 0)
  {
    void* imageContentsTmp = ReadResult.Contents;
    memory_arena* Arena = &aGameState->AssetArena;
    //ReadResult.Contents = PushCopy(Arena, ReadResult.ContentSize, imageContentsTmp);

    bmp_header* Header = (bmp_header*)ReadResult.Contents;//ReadResult.Contents;

    Assert(Header->BitsPerPixel == 32);
    Assert(Header->Compression == 3);

    Result.Pixels = PushArray(Arena, Header->Width*Header->Height, u32);
    Result.Width = Header->Width;
    Result.Height = Header->Height;
    Result.BPP = Header->BitsPerPixel;

    u32* Target = (u32*)Result.Pixels;
    u32* Source = (u32*)(((u8*)ReadResult.Contents) + Header->FileSize - 4);

    bit_scan_result RedBitShift = FindLeastSignificantSetBit(Header->RedMask);
    bit_scan_result GreenBitShift = FindLeastSignificantSetBit(Header->GreenMask);
    bit_scan_result BlueBitShift = FindLeastSignificantSetBit(Header->BlueMask);
    bit_scan_result AlphaBitShift = FindLeastSignificantSetBit(Header->AlphaMask);

    Assert(RedBitShift.Found);
    Assert(GreenBitShift.Found);
    Assert(BlueBitShift.Found);

    for (u32 Y = 0; Y < Result.Height; ++Y)
    {
      for (u32 X = 0; X < Result.Width; ++X)
      {
        u32 Pixel = *Source--;

        bit_scan_result BitShift = {};

        u32 Red = Pixel & Header->RedMask;
        u8 R = (u8)(Red >> RedBitShift.Index);

        u32 Green = Pixel & Header->GreenMask;
        u8 G = (u8)(Green >> GreenBitShift.Index);

        u32 Blue = Pixel & Header->BlueMask;
        u8 B = (u8)(Blue >> BlueBitShift.Index);

        u8 A = 0xff;
        if (AlphaBitShift.Found)
        {
          u32 Alpha = Pixel & Header->AlphaMask;
          A = (u8)(Alpha >> AlphaBitShift.Index);
        }
        *Target++ = (A << 24) | (R << 16) | (G << 8) | (B << 0);

      }
    }

    FreeEntireFile(Thread, imageContentsTmp);
  }

  return Result;
}