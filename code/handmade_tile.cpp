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
	int32 Offset = RoundReal32ToInt32(*TilePos / TileMap->TileSide);

	// The 8 lower bits says which tile we are in a chunk.
	// The other high bits says which chunk we are in a map
	// When we add one only the lower 8 bits are affected
	// When they become greater than 255 the 9th bit flips and
	// we are automaticallty indexed to the first tile in a new chunk. Genious.
	// No if/else statements needed. Everything is handeled automatically.
	*Tile += Offset;
	*TilePos -= Offset*TileMap->TileSide;

	// Assert that we are inside a tile
	Assert(*TilePos <= 0.5 * TileMap->TileSide);
	Assert(*TilePos >= -0.5 * TileMap->TileSide);
}

inline tile_map_position 
RecanonicalizePosition(tile_map* TileMap, tile_map_position CanPos)
{
	tile_map_position Result = CanPos;

	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.X);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Y);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Z);

	return Result;
}

internal tile_map_position 
MoveNewTileMapPosition(tile_map* TileMap, tile_map_position OldCanPos, 
							real32 dx, real32 dy, real32 dz)
{
	tile_map_position TempResult = OldCanPos;
	TempResult.X += dx;
	TempResult.Y += dy;
	TempResult.Z += dz;
	tile_map_position Result = RecanonicalizePosition(TileMap, TempResult);
	return Result;
}

inline tile_pages*
GetTilePage(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{

	tile_pages* Result = 0;
	if( (AbsTileX >= 0) && (AbsTileX < TileMap->PageCountX) && 
		(AbsTileY >= 0) && (AbsTileY < TileMap->PageCountY) && 
		(AbsTileZ >= 0) && (AbsTileZ < TileMap->PageCountZ))
	{
		int32 PageIdx = Get3DIdx(AbsTileX, AbsTileY, AbsTileZ,
			TileMap->PageCountX, TileMap->PageCountY, TileMap->PageCountZ);
		Result  = &TileMap->Map[PageIdx];
	}
	
	return Result;
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
GetTileValueUnchecked(tile_map* TileMap, tile_pages* TilePage, uint32 RelTileIndexX, uint32 RelTileIndexY)
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
SetTileValueUnchecked(tile_map* TileMap, tile_pages* TilePage, uint32 RelTileIndexX, uint32 RelTileIndexY, uint32 TileValue)
{
	Assert(TilePage);
	Assert(RelTileIndexX < TileMap->PageDim);
	Assert(RelTileIndexY < TileMap->PageDim);

	int32 TileIdx = Get3DIdx(RelTileIndexX, RelTileIndexY, 0,
			TileMap->PageDim,TileMap->PageDim, 1);
	TilePage->Page[TileIdx] = TileValue;
}

inline uint32 
GetTileValue(tile_map* TileMap, tile_pages* TilePage, uint32 RelTileIndexX, uint32 RelTileIndexY)
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
	tile_pages* TilePage = GetTilePage(TileMap, TilePos.PageX, TilePos.PageY, TilePos.PageZ);


	if(TilePage && TilePage->Page)
	{
		Result = GetTileValueUnchecked(TileMap, TilePage, TilePos.TileX,TilePos.TileY );
	}
	return Result;
}

inline uint32 GetTileValue(tile_map* TileMap, tile_map_position CanPos)
{
	tile_index TilePosition = GetTileIndex(TileMap, 
			CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);

	tile_pages* TilePage = GetTilePage(TileMap, TilePosition.PageX, 
										TilePosition.PageY, TilePosition.PageZ);

	return GetTileValue(TileMap, TilePage, 
						TilePosition.TileX,
						TilePosition.TileY);
}

inline void
SetTileValueAbs(memory_arena* Arena, tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
	tile_index TilePos = GetTileIndex(TileMap, AbsTileX, AbsTileY, AbsTileZ);
	tile_pages* TilePage = GetTilePage(TileMap, TilePos.PageX, TilePos.PageY, TilePos.PageZ);

	if(!TilePage->Page)
	{
		uint32 TileCount = TileMap->PageDim * TileMap->PageDim;
		TilePage->Page = PushArray(Arena, TileCount, uint32);

		for(uint32 TileIndex = 0; TileIndex < TileCount; TileIndex++)
		{
			TilePage->Page[TileIndex] = 1;
		}
	}

	SetTileValueUnchecked(TileMap, TilePage, TilePos.TileX, TilePos.TileY, TileValue );
}


internal bool32 
IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos)
{
	bool32 Result = true;

	tile_index TilePosition = GetTileIndex(TileMap, 
			CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);

	tile_pages* TilePage = GetTilePage(TileMap, TilePosition.PageX, 
										TilePosition.PageY, TilePosition.PageZ);

	uint32 TileValue  = GetTileValue(TileMap, TilePage, TilePosition.TileX,TilePosition.TileY);
	Result = ( TileValue != 2);
	return Result;
}


