#pragma once

#include "../memory.h"

template <class T>
class vector
{
  T* Base;
  T* FirstFreeSlot;
  u32 MaxCount;

public:
  vector() : Base(0), MaxCount(0){};
  vector(memory_arena* Arena , u32 Size) : Base(0), MaxCount(Size)
  {
    Base = PushArray(Arena, Size, T);
  };

  void Insert(u32 Idx, const T& Value )
  {
    Assert(Idx<MaxCount);
    T* Pos = Base+Idx;
    *Pos = Value;
  }

  T* Get(u32 Idx)
  {
    Assert(Idx<MaxCount);
    return Base+Idx;
  }

  u32 Size()
  {
    return MaxCount;
  }
};