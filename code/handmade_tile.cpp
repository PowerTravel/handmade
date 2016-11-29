#include "handmade_tile.h"

inline uint32
Get3DIdx(int32 x, int32 y, int32 z, int32 X, int32 Y, int32 Z  )
{
	int32 Result = x + y*X + z*X*Y;
	Assert(Result<X*Y*Z);
	Assert(Result >= 0);
	return Result;
}


inline void
RecanonicalizeCoord(tile_map* TileMap, uint32* Tile, real32* TilePos  )
{

	// Since the origin is in the middle of each chunk the offset will be 0 if we are
	// within a chunk (abs(TilePagePos) < 0.5*TilePageSide) but +- 1 otherwise. 
	int32 Offset = RoundReal32ToInt32(*TilePos / TileMap->TileSideInMeters);

	// The 8 lower bits says which tile we are in a page.
	// The other high bits says which chunk we are in a map
	// When we add one only the lower 8 bits are affected
	// When they become greater than 255 the 9th bit flips and
	// we are automaticallty indexed to the first tile in a new chunk. Genious.
	// No if/else statements needed. Everything is handeled automatically.
	*Tile += Offset;
	*TilePos -= Offset*TileMap->TileSideInMeters;

	// Assert that we are inside a tile
	Assert(*TilePos <= 0.5 * TileMap->TileSideInMeters);
	Assert(*TilePos >= -0.5 * TileMap->TileSideInMeters);
}

inline tile_map_position 
RecanonicalizePosition(tile_map* TileMap, tile_map_position CanPos)
{
	tile_map_position Result = CanPos;

	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.RelTile.X);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.RelTile.Y);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.RelTile.Z);

	return Result;
}

internal tile_map_position 
MoveNewTileMapPosition(tile_map* TileMap, tile_map_position OldCanPos, v3 dr)
{
	tile_map_position TempResult = OldCanPos;
	TempResult.RelTile += dr;
	tile_map_position Result = RecanonicalizePosition(TileMap, TempResult);
	return Result;
}

internal tile_map_position 
MoveNewTileMapPosition(tile_map* TileMap, tile_map_position OldCanPos,
							real32 dx, real32 dy, real32 dz)
{
	v3 dr =  V3(dx,dy,dz);
	tile_map_position Result = MoveNewTileMapPosition(TileMap, OldCanPos, dr);
	return Result;
}

#define TILE_PAGE_SAFE_MARGIN 16

inline tile_page*
GetTilePage(tile_map* TileMap, uint32 TilePageX, uint32 TilePageY, uint32 TilePageZ, 
			memory_arena* Arena = 0)
{
	Assert( TilePageX > TILE_PAGE_SAFE_MARGIN);
	Assert( TilePageY > TILE_PAGE_SAFE_MARGIN);
	Assert( TilePageZ > TILE_PAGE_SAFE_MARGIN);
	Assert( TilePageX < (UINT32_MAX-TILE_PAGE_SAFE_MARGIN)); 
	Assert( TilePageY < (UINT32_MAX-TILE_PAGE_SAFE_MARGIN)); 
	Assert( TilePageZ < (UINT32_MAX-TILE_PAGE_SAFE_MARGIN)); 

	// TODO (Jakob): Make a bett er hash function
	uint32 HashValue = 19 * TilePageX + 7*TilePageY + 3 * TilePageZ;
	uint32 HashSlot = HashValue & ( ArrayCount(TileMap->MapHash) - 1);
	Assert( HashSlot < ArrayCount(TileMap->MapHash) );
	
	tile_page* Page = TileMap->MapHash + HashSlot;

	do{
		if( (TilePageX == Page->PageX) &&
			(TilePageY == Page->PageY) &&
			(TilePageZ == Page->PageZ))
		{
			break;
		}

		if( Arena && (Page->PageX != 0) && (!Page->NextInHash) )
		{
			Page->NextInHash = PushStruct(Arena, tile_page );
			Page->PageX=0;
			Page = Page->NextInHash;
			break;
		}

		if( Arena && (Page->PageX == 0) )
		{
			uint32 TileCount = TileMap->PageDim * TileMap->PageDim;

			Page->PageX = TilePageX;
			Page->PageY = TilePageY;
			Page->PageZ = TilePageZ;

			Page->Page = PushArray(Arena, TileCount, uint32);

			Page->NextInHash = 0;
			
			for(uint32 TileIndex = 0; TileIndex < TileCount; TileIndex++)
			{
				Page->Page[TileIndex] = 1;
			}
			Page->NextInHash = 0;
			break;
		}

		Page = Page->NextInHash;

	}while( Page );
	
	return Page;
}

inline tile_index 
GetTileIndex(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY,uint32 AbsTileZ)
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



inline uint32
GetTileValueUnchecked(tile_map* TileMap, tile_page* TilePage, uint32 RelTileIndexX, uint32 RelTileIndexY)
{
 	Assert(TilePage);
	Assert(RelTileIndexX < TileMap->PageDim);
	Assert(RelTileIndexY < TileMap->PageDim);
	int32 TileIdx = Get3DIdx(RelTileIndexX, RelTileIndexY, 0,
			TileMap->PageDim,TileMap->PageDim,1);
	uint32 Result = TilePage->Page[TileIdx];
	return Result;
}

inline void
SetTileValueUnchecked(tile_map* TileMap, tile_page* TilePage, uint32 RelTileIndexX, uint32 RelTileIndexY, uint32 TileValue)
{
	Assert(TilePage);
	Assert(RelTileIndexX < TileMap->PageDim);
	Assert(RelTileIndexY < TileMap->PageDim);

	int32 TileIdx = Get3DIdx(RelTileIndexX, RelTileIndexY, 0,
			TileMap->PageDim,TileMap->PageDim, 1);
	TilePage->Page[TileIdx] = TileValue;
}

inline uint32 
GetTileValue(tile_map* TileMap, tile_page* TilePage, uint32 RelTileIndexX, uint32 RelTileIndexY)
{
	uint32 Result = 0;
	if(TilePage && TilePage->Page)
	{
		Result = GetTileValueUnchecked(TileMap, TilePage, RelTileIndexX,RelTileIndexY);
	}
	return Result;
}


inline uint32 
GetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 
	AbsTileY, uint32 AbsTileZ)
{
	uint32 Result = 0;

	tile_index TilePos = GetTileIndex(TileMap, AbsTileX,AbsTileY,AbsTileZ);
	tile_page* TilePage = GetTilePage(TileMap, TilePos.PageX, TilePos.PageY, TilePos.PageZ);


	if(TilePage && TilePage->Page)
	{
		Result = GetTileValueUnchecked(TileMap, TilePage, TilePos.TileX,TilePos.TileY );
	}
	return Result;
}

inline uint32 
GetTileValue(tile_map* TileMap, tile_map_position CanPos)
{
	tile_index TilePosition = GetTileIndex(TileMap, 
			CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);

	tile_page* TilePage = GetTilePage(TileMap, TilePosition.PageX, 
										TilePosition.PageY, TilePosition.PageZ);

	return GetTileValue(TileMap, TilePage, 
						TilePosition.TileX,
						TilePosition.TileY);
}

inline void
SetTileValueAbs(memory_arena* Arena, tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
	tile_index TilePos = GetTileIndex(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_page* TilePage = GetTilePage(TileMap, TilePos.PageX, TilePos.PageY, TilePos.PageZ, Arena);
	SetTileValueUnchecked(TileMap, TilePage, TilePos.TileX, TilePos.TileY, TileValue );
}


internal bool32 
IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos)
{
	bool32 Result = true;

	tile_index TilePosition = GetTileIndex(TileMap, 
			CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);

	tile_page* TilePage = GetTilePage(TileMap, TilePosition.PageX, 
										TilePosition.PageY, TilePosition.PageZ);

	uint32 TileValue  = GetTileValue(TileMap, TilePage, TilePosition.TileX,TilePosition.TileY);
	Result = ( TileValue != 2);
	return Result;
}


internal void
InitializeTileMap( tile_map* TileMap, real32 TileSideInMeters  )
{
	TileMap->TileSideInMeters = 1.4f; 
	TileMap->PageShift = 4;
	TileMap->PageMask = (1<<TileMap->PageShift)-1;		
	TileMap->PageDim = (1<<TileMap->PageShift);

	for(uint32 TilePageIndex = 0;
		TilePageIndex < ArrayCount(TileMap->MapHash);
		++TilePageIndex)
	{
		TileMap->MapHash[TilePageIndex].PageX = 0;
	}
}
