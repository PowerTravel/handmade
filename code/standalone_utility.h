#pragma once

#define CopyArray( Count, Source, Dest ) utils::Copy( (Count)*sizeof( *(Source) ), ( Source ), ( Dest ) )

namespace utils
{
  inline void* Copy(memory_index aSize, void* SourceInit, void* DestInit)
  {
      u8 *Source = (u8 *)SourceInit;
      u8 *Dest = (u8 *)DestInit;
      while(aSize--) {*Dest++ = *Source++;}

      return(DestInit);
  }

  class push_buffer
  {
    u8* Base;
    u8* Tail;
    memory_index Size;

    public:

      push_buffer():
        Base(0),
        Size(0),
        Tail(0)
      {};

      push_buffer(u8* MemStart, memory_index MemSize) :
        Base(MemStart),
        Size(MemSize),
        Tail(MemStart)
      {};

      b32 IsEmpty()
      {
        return (Base == Tail);
      }

      memory_index Remaining()
      {
        return (Base + Size) - Tail;
      }

      u8* GetBase()
      {
        return Base;
      }

      u8* GetTail()
      {
        return Tail;
      }
      memory_index GetUsed(){ return Tail - Base;};
      u8* GetMemory( memory_index MemoryCount )
      {
        if(MemoryCount >= Remaining()) {return 0;}
        u8* Result = Tail;
        Tail += MemoryCount;
        return Result;
      };

      void Clear()
      {
        Tail = Base;
      }
    };

}
