#include "handmade_tile.h"

inline u32
Get3DIdx(s32 x, s32 y, s32 z, s32 X, s32 Y, s32 Z  )
{
  s32 Result = x + y*X + z*X*Y;
  Assert(Result<X*Y*Z);
  Assert(Result >= 0);
  return Result;
}

inline void
RecanonicalizeCoord(tile_map* TileMap, u32* Tile, r32* TilePos, r32 TileSideInMeters  )
{
  // Note(Jakob): Using floor sets the Origin of tile is in the lower corner.
  //              I changed from using Round because it caused the border of the tile
  //              to be undefined. A tile position AbsTile 1 and RelTile 0.5 would flip
  //              flop between AbsTile 2, RelTile -0.5 and AbsTile 1 and RelTile 0.5 by
  //              Successive calls to RecanonicalizeCoord;
  s32 Offset = (s32) Floor(*TilePos / TileSideInMeters);

  *Tile += Offset;
  *TilePos -= Offset*TileSideInMeters;

  // Assert that we are inside a tile
  Assert( *TilePos >=  0 );
  Assert( *TilePos < TileSideInMeters );
}

inline tile_map_position
RecanonicalizePosition(tile_map* TileMap, tile_map_position CanPos)
{
  tile_map_position Result = CanPos;

  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.RelTileX, TileMap->TileWidthInMeters );
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.RelTileY, TileMap->TileHeightInMeters );
  RecanonicalizeCoord(TileMap, &Result.AbsTileZ, &Result.RelTileZ, TileMap->TileDepthInMeters );

  return Result;
}

inline tile_map_position
CanonicalizePosition( tile_map* TileMap, v3 Pos )
{
  tile_map_position CanPos = {};
  CanPos.RelTileX = Pos.X;
  CanPos.RelTileY = Pos.Y;
  CanPos.RelTileZ = Pos.Z;
  tile_map_position Result = RecanonicalizePosition(TileMap, CanPos);
  return Result;
}

internal tile_map_position
MoveNewTileMapPosition(tile_map* TileMap, tile_map_position OldCanPos, r32 dx, r32 dy, r32 dz)
{
  tile_map_position TempResult = OldCanPos;
  TempResult.RelTileX += dx;
  TempResult.RelTileY += dy;
  TempResult.RelTileZ += dz;
  tile_map_position Result = RecanonicalizePosition(TileMap, TempResult);
  return Result;
}

aabb3f GetTileAABB(tile_map* TileMap, tile_map_position CanPos )
{

  r32 HalfWidth  = TileMap->TileWidthInMeters;
  r32 HalfHeight = TileMap->TileHeightInMeters;
  r32 HalfDepth  = TileMap->TileDepthInMeters;

  // Lower Left Back
  v3 P0 = V3( CanPos.AbsTileX*TileMap->TileWidthInMeters ,
        CanPos.AbsTileY*TileMap->TileHeightInMeters,
        CanPos.AbsTileZ*TileMap->TileDepthInMeters );

  // Upper Right Front
  v3 P1 = V3( P0.X + TileMap->TileWidthInMeters ,
        P0.Y + TileMap->TileHeightInMeters,
        P0.Z /* Tilemap in Z direction is 2d Slices */);

  aabb3f Result = AABB3f(P0,P1);

  return Result;
}

#define TILE_PAGE_SAFE_MARGIN (INT32_MAX/64)
#define TILE_PAGE_UNINITIALIZED INT32_MAX

inline tile_page*
GetTilePage(tile_map* TileMap, s32 TilePageX, s32 TilePageY, s32 TilePageZ,
      memory_arena* Arena = 0)
{
  Assert( TilePageX > -TILE_PAGE_SAFE_MARGIN);
  Assert( TilePageY > -TILE_PAGE_SAFE_MARGIN);
  Assert( TilePageZ > -TILE_PAGE_SAFE_MARGIN);
  Assert( TilePageX <  TILE_PAGE_SAFE_MARGIN);
  Assert( TilePageY <  TILE_PAGE_SAFE_MARGIN);
  Assert( TilePageZ <  TILE_PAGE_SAFE_MARGIN);

  // TODO (Jakob): Make a bett er hash function (lol)
  u32 HashValue = 19 * TilePageX + 7*TilePageY + 3 * TilePageZ;
  u32 HashSlot = HashValue & ( ArrayCount(TileMap->MapHash) - 1);
  Assert( HashSlot < ArrayCount(TileMap->MapHash) );

  tile_page* Page = TileMap->MapHash + HashSlot;

  do{
    if( (TilePageX == Page->PageX) &&
      (TilePageY == Page->PageY) &&
      (TilePageZ == Page->PageZ))
    {
      break;
    }

    if( Arena && ( Page->PageX != TILE_PAGE_UNINITIALIZED ) && ( !Page->NextInHash ) )
    {
      Page->NextInHash = PushStruct( Arena, tile_page );
      Page->NextInHash->PageX = TILE_PAGE_UNINITIALIZED;
      Page = Page->NextInHash;
      break;
    }

    if( Arena && ( Page->PageX == TILE_PAGE_UNINITIALIZED ) )
    {
      u32 TileCount = TileMap->PageDim * TileMap->PageDim;

      Page->PageX = TilePageX;
      Page->PageY = TilePageY;
      Page->PageZ = TilePageZ;

      // For now a page is just a 2d grid of tile_contents
      Page->Page = PushArray(Arena, TileCount, tile_contents);

      Page->NextInHash = 0;

      for( u32 TileIndex = 0; TileIndex < TileCount; ++TileIndex )
      {
        Page->Page[TileIndex] = {};
      }
      break;
    }

    Page = Page->NextInHash;

  }while( Page );

  return Page;
}

inline tile_index
GetTileIndex(tile_map* TileMap, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ)
{
  tile_index Result = {};

  // Get high bits out by shifting AbsTile 8 bits to the right
  Result.PageX = AbsTileX >> TileMap->PageShift;
  Result.PageY = AbsTileY >> TileMap->PageShift;
  Result.PageZ = AbsTileZ;

  // Get Low bits out by masking away the high bits
  Result.TileX = AbsTileX & TileMap->PageMask;
  Result.TileY = AbsTileY & TileMap->PageMask;

  return Result;
}



inline tile_contents
GetTileContentsUnchecked(tile_map* TileMap, tile_page* TilePage, s32 RelTileIndexX, s32 RelTileIndexY )
{
  Assert(TilePage);
  Assert(RelTileIndexX < TileMap->PageDim);
  Assert(RelTileIndexY < TileMap->PageDim);

  s32 TileIdx = Get3DIdx(RelTileIndexX, RelTileIndexY, 0,
      TileMap->PageDim,TileMap->PageDim,1);
  tile_contents Result = TilePage->Page[TileIdx];
  return Result;
}

inline void
SetTileContentsUnchecked(tile_map* TileMap, tile_page* TilePage, s32 RelTileIndexX, s32 RelTileIndexY, tile_contents TileContents)
{
  Assert(TilePage);
  Assert(RelTileIndexX < TileMap->PageDim);
  Assert(RelTileIndexY < TileMap->PageDim);

  s32 TileIdx = Get3DIdx(RelTileIndexX, RelTileIndexY, 0,
      TileMap->PageDim,TileMap->PageDim, 1);

  TilePage->Page[TileIdx] = TileContents;
}

inline tile_contents
GetTileContents(tile_map* TileMap, tile_page* TilePage, s32 RelTileIndexX, s32 RelTileIndexY )
{
  tile_contents Result = {};
  if(TilePage && TilePage->Page)
  {
    Result = GetTileContentsUnchecked(TileMap, TilePage, RelTileIndexX, RelTileIndexY);
  }
  return Result;
}


inline tile_contents
GetTileContents(tile_map* TileMap, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ)
{
  tile_contents Result = {};

  tile_index TilePos  = GetTileIndex(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_page* TilePage = GetTilePage(TileMap, TilePos.PageX, TilePos.PageY, TilePos.PageZ);

  if(TilePage && TilePage->Page)
  {
    Result = GetTileContentsUnchecked(TileMap, TilePage, TilePos.TileX, TilePos.TileY );
  }
  return Result;
}

inline tile_contents
GetTileContents(tile_map* TileMap, tile_map_position CanPos)
{
  tile_index TilePosition = GetTileIndex(TileMap,
      CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);

  tile_page* TilePage = GetTilePage(TileMap, TilePosition.PageX,
                    TilePosition.PageY, TilePosition.PageZ);

  return GetTileContents(TileMap, TilePage,
               TilePosition.TileX,
               TilePosition.TileY);
}

inline void
SetTileContentsAbs(memory_arena* Arena, tile_map* TileMap, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ, tile_contents TileContents)
{
  tile_index TilePos = GetTileIndex(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_page* TilePage = GetTilePage(TileMap, TilePos.PageX, TilePos.PageY, TilePos.PageZ, Arena);
  SetTileContentsUnchecked(TileMap, TilePage, TilePos.TileX, TilePos.TileY, TileContents );
}


internal b32
IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos)
{
  tile_index TilePosition = GetTileIndex(TileMap,
      CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);

  tile_page* TilePage = GetTilePage(TileMap, TilePosition.PageX,
                    TilePosition.PageY, TilePosition.PageZ);


  tile_contents TileContents  = GetTileContents(TileMap, TilePage, TilePosition.TileX, TilePosition.TileY);

  return TileContents.Type == TILE_TYPE_NONE;
}

void InitializeTileMap( tile_map* TileMap )
{
  TileMap->TileWidthInMeters = 1.f;
  TileMap->TileHeightInMeters = 1.f;
  TileMap->TileDepthInMeters = 1.f;
  TileMap->PageShift = 4;
  TileMap->PageMask = (1<<TileMap->PageShift)-1;
  TileMap->PageDim = (1<<TileMap->PageShift);

  for( u32 TilePageIndex = 0;
     TilePageIndex < ArrayCount(TileMap->MapHash);
     ++TilePageIndex)
  {
    TileMap->MapHash[TilePageIndex].PageX = TILE_PAGE_UNINITIALIZED;
  }
}


void GetIntersectingTiles(tile_map* TileMap, list<tile_map_position>* OutputList, aabb3f* AABB )
{
  s32 MinXIdx = (s32) Floor(AABB->P0.X / TileMap->TileWidthInMeters);
  s32 MinYIdx = (s32) Floor(AABB->P0.Y / TileMap->TileHeightInMeters);
  s32 MinZIdx = (s32) Floor(AABB->P0.Z / TileMap->TileDepthInMeters);
  s32 MaxXIdx = (s32) Floor(AABB->P1.X / TileMap->TileWidthInMeters);
  s32 MaxYIdx = (s32) Floor(AABB->P1.Y / TileMap->TileHeightInMeters);
  s32 MaxZIdx = (s32) Floor(AABB->P1.Z / TileMap->TileDepthInMeters);
  OutputList->First();
  for( s32 IdxZ = MinZIdx; IdxZ <= MaxZIdx; ++IdxZ  )
  {
    r32 Z = IdxZ * TileMap->TileDepthInMeters;
    for( s32 IdxY = MinYIdx; IdxY <= MaxYIdx; ++IdxY  )
    {
      r32 Y = IdxY * TileMap->TileHeightInMeters;
      for( s32 IdxX = MinXIdx; IdxX <= MaxXIdx; ++IdxX  )
      {
        r32 X = IdxX * TileMap->TileWidthInMeters;
        tile_map_position TilePos = CanonicalizePosition( TileMap, V3( X, Y, Z ) );
        OutputList->InsertAfter(TilePos);
      }
    }
  }
}