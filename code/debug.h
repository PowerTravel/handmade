#pragma once

#include "types.h"
#include "intrinsics.h"
#include "memory.h"
#include "string.h"

#pragma intrinsic(__rdtsc)

struct debug_state;

internal debug_state* DEBUGGetState();

struct debug_record_entry
{
  u32 LineNumber;
  char BlockName[256];
  u32 CycleCount;
  r32 HitCount;
  r32 HCCount;
};

struct debug_statistics
{
  r32 HitCount;
  r32 Min;
  r32 Max;
  r32 Tot;
  debug_record_entry* Record;
};

struct list_entry
{
  list_entry* Next;
  list_entry* Previous;
};

struct vector_list
{
  midx EntryDataSize;
  u32 VectorMaxCount;

  u32 ListCount;
  list_entry Sentinel;
  list_entry* FreeEntry;
  bptr Bytes;
};

inline midx GetRequiredVectorListMemoryByteCount(u32 VectorMaxCount, midx EntryDataSize)
{
  midx Result = VectorMaxCount * (EntryDataSize + sizeof(list_entry));
  return Result;
}

vector_list* _BeginVectorList(void* Memory, u32 VectorMaxCount, midx EntryDataSize)
{
  vector_list* Result = (vector_list*) Memory;
  Result->VectorMaxCount = VectorMaxCount;
  Result->EntryDataSize = EntryDataSize;
  Result->Bytes = ((bptr) Memory) + sizeof(vector_list);
  ListInitiate(&Result->Sentinel);
  return Result;
};

vector_list* _BeginVectorList( memory_arena* Arena, u32 VectorMaxCount, midx EntryDataSize)
{
  midx MemorySize = GetRequiredVectorListMemoryByteCount(VectorMaxCount, EntryDataSize);
  void* Memory = PushSize(Arena, MemorySize + sizeof(vector_list));
  vector_list* Result = _BeginVectorList(Memory, VectorMaxCount, EntryDataSize);
  return Result;
};

#define BeginVectorList(MemorySource, VectorMaxCount, Type) _BeginVectorList(MemorySource, VectorMaxCount, sizeof(Type))

list_entry* GetEntry(vector_list* List, u32 EntryIndex)
{
  Assert(EntryIndex < List->VectorMaxCount);
  midx Stride = List->EntryDataSize + sizeof(list_entry);
  midx ByteOffset = Stride * EntryIndex;
  list_entry* Result = (list_entry*) (List->Bytes + ByteOffset); 
  return Result;
}

inline void* GetDataFromEntry(list_entry* Entry)
{
  void* Result = (void*) (((bptr) Entry) + sizeof(list_entry));
  return Result;
}

inline list_entry* GetEntryFromData(void* Data)
{
  list_entry* Result = (list_entry*) (((bptr) Data) - sizeof(list_entry));
  return Result;
}

inline b32 IsEmpty(vector_list* List)
{
  Assert(List->Sentinel.Next);
  b32 Result = (List->Sentinel.Next == &List->Sentinel);
  return Result;
}

inline b32 IsEnd(vector_list* List, void* Data)
{
  list_entry* Entry = 0;
  b32 Result = true;
  if(Data)
  {
    Entry = GetEntryFromData(Data);
    Result = (Entry == &List->Sentinel);
  }

  return Result;
}


inline void* First(vector_list* List)
{
  void* Result = GetDataFromEntry(List->Sentinel.Next);
  return Result;
}

inline void* Next(vector_list* List, void* Data)
{
  void* Result = 0;
  list_entry* Entry = GetEntryFromData(Data);
  Entry = Entry->Next;
  if(!IsEnd(List, Entry))
  {
    Result = GetDataFromEntry(Entry);
  }
  return Result;
}


void ClearVectorList(vector_list* List)
{
  TIMED_FUNCTION();
  #if 0
  midx ByteSize = GetRequiredVectorListMemoryByteCount(List->VectorMaxCount, List->EntryDataSize);
  utils::ZeroSize(ByteSize, List->Bytes);
  ListInitiate(&List->Sentinel);
  #else
  void* Data = First(List);
  while(!IsEnd(List, Data))
  {
    list_entry* Entry = GetEntryFromData(Data);
    Data = Next(List, Data);

    ListRemove(Entry);
    utils::ZeroSize(List->EntryDataSize + sizeof(list_entry), Entry);
  }
  List->ListCount = 0;
  #endif
}

inline void* GetEntryData(vector_list* List, u32 EntryIndex)
{
  list_entry* Entry = GetEntry(List, EntryIndex);
  void* Result = GetDataFromEntry(Entry);
  return Result;
}

inline b32 Exists(vector_list* List, u32 EntryIndex, void** DataResult = 0)
{
  list_entry* Entry = GetEntry(List, EntryIndex);
  b32 Result = Entry->Next != 0;
  if(Result && DataResult)
  {
    *DataResult = GetDataFromEntry(Entry);
  }
  return Result;
}

inline void* PushBack(vector_list* List, u32 EntryIndex,  void* Data)
{
  list_entry* Entry = GetEntry(List, EntryIndex);
  void* Result = GetDataFromEntry(Entry);
  utils::Copy(List->EntryDataSize, Data, Result);

  List->ListCount++;
  ListInsertBefore(&List->Sentinel, Entry);

  return Result;
}


u32 GetIndexOfEntry(vector_list* List, void* Data)
{
  list_entry* Entry = GetEntryFromData(Data);
  bptr ByteEntry = (bptr) Entry;
  midx ByteOffset = ByteEntry - List->Bytes;
  midx ByteStride = (midx) List->EntryDataSize + sizeof(list_entry);
  u32 Result = (u32) (ByteOffset/ByteStride);
  return Result;
}

// The information for the frame
#define MAX_BLOCKS_PER_FRAME 4096
#define MAX_THREAD_COUNT 16
#define MAX_DEBUG_FRAME_COUNT 60

struct debug_block
{
  debug_record_entry* Record;
  u32 ThreadIndex;
  u64 BeginClock;
  u64 EndClock;
  debug_event OpeningEvent;
  debug_block* Parent;
  debug_block* FirstChild;
  debug_block* Next;
};

struct debug_thread
{
  u32 ID;
  u32 LaneIndex;
  debug_block* FirstBlock;
  debug_block* OpenBlock;
  debug_block* ClosedBlock;
  debug_block* SelectedBlock;
};

struct debug_frame
{
  u64 BeginClock;
  u64 EndClock;
  r32 WallSecondsElapsed;

  u32 FrameBarLaneCount;

  midx MaxBlockCount;
  debug_block* Blocks;
  debug_block* FirstFreeBlock;

  debug_thread Threads[MAX_THREAD_COUNT];

  vector_list* Statistics;
};


struct debug_state
{
  b32 Initialized;

  b32 Paused;

  memory_arena Arena;
  temporary_memory StatisticsTemp;

  b32 Compiling;
  debug_executing_process Compiler;

  b32 ConfigMultiThreaded;
  b32 ConfigCollisionPoints;
  b32 ConfigCollider;
  b32 ConfigAABBTree;

  // This is a rolling buffer that holds all data for all frames
  u32 CurrentFrameIndex;
  debug_frame* SelectedFrame;
  
  b32 ThreadSelected;
  u32 SelectedThreadIndex;
  debug_frame Frames[MAX_DEBUG_FRAME_COUNT];

  // Keeps a global record of all seen functions and their execution time and hit cout.
  vector_list* FunctionList;
};

inline void DebugRewriteConfigFile();
void PushDebugOverlay(game_input* GameInput);

