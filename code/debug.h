#pragma once

#include "types.h"
#include "intrinsics.h"
#include "memory.h"
#include <stdio.h>
#pragma intrinsic(__rdtsc)

#if HANDMADE_INTERNAL
#define TIMED_BLOCK__(Number, ... ) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__);
#define TIMED_BLOCK_(Number, ... ) TIMED_BLOCK__(Number, ##_VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ##_VA_ARGS__)
#else
#define TIMED_BLOCK;
#endif

struct debug_record
{
  char* FileName;
  char* FunctionName;

  u32 LineNumber;
  u64 HitCount_CycleCount;
  u32 Padding;
};

enum debug_event_type
{
  DebugEvent_BeginBlock,
  DebugEvent_EndBlock,
};

struct debug_event
{
  u64 Clock;
  u32 ThreadIndex;
  //u16 CoreIndex;
  u16 DebugRecordIndex;
  //u8 DebugRecordArrayIndex;
  u8 Type;
  u8 Padding;
};

extern debug_record GlobalDebugRecordArray[];

#define MAX_DEBUG_EVENT_COUNT (65536)
extern u64 Global_DebugEventArrayIndex_DebugEventIndex;
extern debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];

inline void RecordDebugEvent(u32 RecordIndex, u32 EventType)
{
  u64 ArrayIndex_EventIndex = AtomicAddu64(&Global_DebugEventArrayIndex_DebugEventIndex, 1);
  u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF;
  u32 ArrayIndex = ArrayIndex_EventIndex >> 32;
  Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);
  debug_event* Event = GlobalDebugEventArray[ArrayIndex] + EventIndex;
  Event->Clock = __rdtsc();
  Event->ThreadIndex = GetThreadID();
  //Event->CoreIndex = 0;
  Event->DebugRecordIndex = (u16) RecordIndex;
  // Careful, this is defined in uniquely in each compilation unit in buld.bat as an identifier
  // So this function will be defined once for each compilation unit with each one having a unique
  // value for DebugRecordArrayIndexConstant.
  // HOWEVER when the linker comes along it will only use ONE of the definitions.
  // To fix this issue make this into a macro again. It's only an inline func now so we can step through it in VS.
  // Event->DebugRecordArrayIndex = DebugRecordArrayIndexConstant;
  Event->Type = (u8) EventType;
}


struct timed_block
{
  debug_record* Record;
  u64 StartCycles;
  u32 HitCount;
  u32 DebugRecordIndex;
  timed_block(const u32 RecordIndex, const char* FileName, const u32 LineNumber, const char* FunctionName, u32 HitCountInit = 1)
  : StartCycles(__rdtsc()), HitCount(HitCountInit), DebugRecordIndex(RecordIndex)
  {
    Record = GlobalDebugRecordArray + DebugRecordIndex;
    Record->FileName     = (char*) FileName;
    Record->FunctionName = (char*) FunctionName;
    Record->LineNumber   = LineNumber;

    RecordDebugEvent(DebugRecordIndex, DebugEvent_BeginBlock);
  };

  ~timed_block()
  {
    u64 DeltaTime = (__rdtsc() - StartCycles) | ((u64)HitCount << 32);
    AtomicAddu64(&Record->HitCount_CycleCount, DeltaTime);
    RecordDebugEvent(DebugRecordIndex, DebugEvent_EndBlock);
  };
};

struct debug_statistics
{
  r32 Count;
  r32 Min;
  r32 Max;
  r32 Avg;
};

struct debug_frame_snapshot
{
  debug_statistics HitCountStat;
  debug_statistics CycleCountStat;
  u32 HitCount;
  u64 CycleCount;
};

struct debug_frame_timestamp
{
  char* Name;
  r32 Seconds;
};

struct debug_frame_end_info
{
  r32 TotalSeconds;
  u32 TimestampCount;
  debug_frame_timestamp Timestamps[64];
};

#define SNAPSHOT_COUNT 128
struct debug_counter_state
{
  char* FileName;
  char* FunctionName;
  u32 LineNumber;
  debug_frame_snapshot Snapshots[SNAPSHOT_COUNT];
};

struct debug_state
{
  memory_arena Memory;
  u32 SnapShotIndex;
  u32 CounterStateCount;
  debug_counter_state CounterStates[512];

  debug_frame_end_info FrameEndInfos[SNAPSHOT_COUNT];
};
