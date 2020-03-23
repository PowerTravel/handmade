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
#define TIMED_BLOCK __COUNTER__;
#endif

struct debug_record
{
  char* FileName;
  char* FunctionName;

  u32 LineNumber;
  u64 HitCount_CycleCount;
  u32 Padding;
};

extern debug_record DebugRecordArray[];

struct timed_block
{
  debug_record* Record;
  u64 StartCycles;
  u32 HitCount;
  timed_block(const u32 Counter, const char* FileName, const u32 LineNumber, const char* FunctionName, u32 HitCountInit = 1)
  : StartCycles(__rdtsc()), HitCount(HitCountInit)
  {
    Record = DebugRecordArray + Counter;
    Record->FileName     = (char*) FileName;
    Record->FunctionName = (char*) FunctionName;
    Record->LineNumber   = LineNumber;
  };

  ~timed_block()
  {
    u64 DeltaTime = (__rdtsc() - StartCycles) | ((u64)HitCount << 32);
    AtomicAddu64(&Record->HitCount_CycleCount, DeltaTime);
  };
};

struct debug_counter_snapshot
{
  u32 HitCount;
  u32 CycleCount;
};

struct debug_counter_state
{
  char* FileName;
  char* FunctionName;
  u32 LineNumber;
  debug_counter_snapshot Snapshots[120];
};

struct debug_state
{
  memory_arena Memory;
  debug_counter_state State[512];
};