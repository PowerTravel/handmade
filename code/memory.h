#pragma once

#include "platform.h"
#include "utility_macros.h"

struct memory_arena
{
  platform_memory_block *CurrentBlock;
  uintptr_t MinimumBlockSize;

  u64 AllocationFlags;
  s32 TempCount;
};

struct temporary_memory
{
  memory_arena *Arena;
  platform_memory_block *Block;
  uintptr_t Used;
};

inline void
SetMinimumBlockSize( memory_arena *Arena, midx MinimumBlockSize )
{
  Arena->MinimumBlockSize = MinimumBlockSize;
}

inline midx
GetAlignmentOffset( memory_arena *Arena, midx Alignment )
{
  midx AlignmentOffset = 0;

  midx ResultPointer = (midx)Arena->CurrentBlock->Base + Arena->CurrentBlock->Used;
  midx AlignmentMask = Alignment - 1;
  if(ResultPointer & AlignmentMask)
  {
      AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
  }

  return(AlignmentOffset);
}

enum arena_push_flag
{
  ArenaFlag_ClearToZero = 0x1,
};

struct arena_push_params
{
  u32 Flags;
  u32 Alignment;
};

inline arena_push_params
DefaultArenaParams( void )
{
  arena_push_params Params;
  Params.Flags = ArenaFlag_ClearToZero;
  Params.Alignment = 4;
  return(Params);
}

inline arena_push_params
AlignNoClear( u32 Alignment )
{
  arena_push_params Params = DefaultArenaParams();
  Params.Flags &= ~ArenaFlag_ClearToZero;
  Params.Alignment = Alignment;
  return(Params);
}

inline arena_push_params
Align( u32 Alignment, b32 Clear )
{
  arena_push_params Params = DefaultArenaParams();
  if(Clear)
  {
    Params.Flags |= ArenaFlag_ClearToZero;
  }
  else
  {
    Params.Flags &= ~ArenaFlag_ClearToZero;
  }
  Params.Alignment = Alignment;
  return(Params);
}

inline arena_push_params
NoClear( void )
{
  arena_push_params Params = DefaultArenaParams();
  Params.Flags &= ~ArenaFlag_ClearToZero;
  return(Params);
}

struct arena_bootstrap_params
{
  u64 AllocationFlags;
  uintptr_t MinimumBlockSize;
};

inline arena_bootstrap_params
DefaultBootstrapParams( void )
{
  arena_bootstrap_params Params = {};
  return(Params);
}

inline arena_bootstrap_params
NonRestoredArena( void )
{
  arena_bootstrap_params Params = DefaultBootstrapParams();
  Params.AllocationFlags = PlatformMemory_NotRestored;
  return(Params);
}

// TODO(casey): Optional "clear" parameter!!!!
#define PushStruct(Arena, type, ...)                   (type *)PushSize_( Arena,         sizeof(type), ## __VA_ARGS__ )
#define PushArray(Arena, Count, type, ...)             (type *)PushSize_( Arena, (Count)*sizeof(type), ## __VA_ARGS__ )
#define PushSize(Arena, Size, ...)                             PushSize_( Arena, Size,                 ## __VA_ARGS__ )
#define PushCopy(Arena, Size, Source, ...)  utils::Copy(Size, Source, PushSize_( Arena, Size,          ## __VA_ARGS__ ))

inline midx
GetEffectiveSizeFor( memory_arena *Arena, midx SizeInit, arena_push_params Params = DefaultArenaParams() )
{
  midx Size = SizeInit;

  midx AlignmentOffset = GetAlignmentOffset( Arena, Params.Alignment );
  Size += AlignmentOffset;

  return( Size );
}

inline void *
PushSize_( memory_arena *Arena, midx SizeInit, arena_push_params Params = DefaultArenaParams() )
{
  void *Result = 0;

  midx Size = 0;
  if( Arena->CurrentBlock )
  {
    Size = GetEffectiveSizeFor( Arena, SizeInit, Params );
  }

  if( !Arena->CurrentBlock ||
     ( Arena->CurrentBlock->Used + Size) > Arena->CurrentBlock->Size )
  {
    Size = SizeInit; // NOTE(casey): The base will automatically be aligned now!
    if(Arena->AllocationFlags & ( PlatformMemory_OverflowCheck |
                                  PlatformMemory_UnderflowCheck ) )
    {
      Arena->MinimumBlockSize = 0;
      Size = AlignPow2( Size, Params.Alignment );
    }
    else if(!Arena->MinimumBlockSize)
    {
      // TODO(casey): Tune default block size eventually?
      Arena->MinimumBlockSize = 1024*1024;
    }

    midx BlockSize = Maximum( Size, Arena->MinimumBlockSize );

    platform_memory_block *NewBlock =
        Platform.AllocateMemory( BlockSize, Arena->AllocationFlags );
    NewBlock->ArenaPrev = Arena->CurrentBlock;
    Arena->CurrentBlock = NewBlock;
  }

  Assert( ( Arena->CurrentBlock->Used + Size ) <= Arena->CurrentBlock->Size );

  midx AlignmentOffset = GetAlignmentOffset( Arena, Params.Alignment );
  Result = Arena->CurrentBlock->Base + Arena->CurrentBlock->Used + AlignmentOffset;
  Arena->CurrentBlock->Used += Size;

  Assert( Size >= SizeInit );

  if( Params.Flags & ArenaFlag_ClearToZero )
  {
    utils::ZeroSize( SizeInit, Result );
  }

  return( Result );
}

inline temporary_memory
BeginTemporaryMemory( memory_arena *Arena )
{
  temporary_memory Result;

  Result.Arena = Arena;
  Result.Block = Arena->CurrentBlock;
  Result.Used =  Arena->CurrentBlock ? Arena->CurrentBlock->Used : 0;

  ++Arena->TempCount;

  return( Result );
}

inline void
FreeLastBlock( memory_arena *Arena )
{
  platform_memory_block *Free = Arena->CurrentBlock;
  Arena->CurrentBlock = Free->ArenaPrev;
  Platform.DeallocateMemory( Free );
}

inline void
EndTemporaryMemory( temporary_memory TempMem )
{
  memory_arena *Arena = TempMem.Arena;
  while( Arena->CurrentBlock != TempMem.Block )
  {
      FreeLastBlock( Arena );
  }

  if( Arena->CurrentBlock )
  {
      Assert( Arena->CurrentBlock->Used >= TempMem.Used );
      Arena->CurrentBlock->Used = TempMem.Used;
      Assert( Arena->TempCount > 0 );
  }
  --Arena->TempCount;
}

inline void
Clear( memory_arena *Arena )
{
  while( Arena->CurrentBlock )
  {
    // NOTE(casey): Because the arena itself may be stored in the last block,
    // we must ensure that we don't look at it after freeing.
    b32 ThisIsLastBlock = ( Arena->CurrentBlock->ArenaPrev == 0 );
    FreeLastBlock( Arena );
    if( ThisIsLastBlock )
    {
      break;
    }
  }
}

struct scoped_temporary_memory
{
  temporary_memory TempMem;

  scoped_temporary_memory(memory_arena* Arena)
  {
    TempMem = BeginTemporaryMemory( Arena );
  };

  ~scoped_temporary_memory()
  {
    EndTemporaryMemory( TempMem );
  };
};

inline void
CheckArena( memory_arena *Arena )
{
  Assert( Arena->TempCount == 0 );
}

struct ScopedMemory
{
  temporary_memory TempMem;
  ScopedMemory(memory_arena* Arena)
  {
    TempMem = BeginTemporaryMemory(Arena);
  }
  ~ScopedMemory()
  {
    EndTemporaryMemory(TempMem);
  }
};

#define BootstrapPushStruct( type, Member, ... ) (type*) BootstrapPushSize_( sizeof( type ), OffsetOf(type, Member), ## __VA_ARGS__ )
inline void *
BootstrapPushSize_( uintptr_t StructSize, uintptr_t OffsetToArena,
                    arena_bootstrap_params BootstrapParams = DefaultBootstrapParams(),
                    arena_push_params Params = DefaultArenaParams() )
{
  memory_arena Bootstrap = {};
  Bootstrap.AllocationFlags = BootstrapParams.AllocationFlags;
  Bootstrap.MinimumBlockSize = BootstrapParams.MinimumBlockSize;
  void* Struct = PushSize( &Bootstrap, StructSize, Params );
  *(memory_arena*) ( (u8*) Struct + OffsetToArena ) = Bootstrap;

  return(Struct);
}