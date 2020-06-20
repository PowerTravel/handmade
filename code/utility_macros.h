#pragma once

#include "intrinsics.h"


#define GetCantorPair(a,b) ((u32) ( (1/2.f) * ((a) + (b)) * ((a) + (b) + 1) + (b) ))
#define Clamp(Val, Min, Max) ((Val) < (Min)) ? (Min) : (((Val) > (Max)) ? (Max) : (Val))

#define DoubleLinkListInitiate( Sentinel ) \
{                                          \
  Sentinel->Previous = Sentinel;           \
  Sentinel->Next = Sentinel;               \
}

#define DoubleLinkListInsertAfter( Sentinel, Element ) \
{                                        \
  Element->Previous = Sentinel;          \
  Element->Next = Sentinel->Next;        \
  Element->Previous->Next = Element;     \
  Element->Next->Previous = Element;     \
}

#define DoubleLinkListInsertBefore( Sentinel, Element ) \
{                                         \
  Element->Previous = Sentinel->Previous; \
  Element->Next = Sentinel;               \
  Element->Previous->Next = Element;      \
  Element->Next->Previous = Element;      \
}

#define Maximum(A, B) ((A > B) ? (A) : (B))
#define Minimum(A, B) ((A < B) ? (A) : (B))

#define GetAbsoluteMax(a,b) ( Abs(a) > Abs(b) ) ? Abs(a) : Abs(b);
#define GetAbsoluteMin(a,b) ( Abs(a) < Abs(b) ) ? Abs(a) : Abs(b);

#define OffsetOf(type, Member) (umm) &(((type *)0)->Member)

#define CopyArray( Count, Source, Dest ) utils::Copy( (Count)*sizeof( *(Source) ), ( Source ), ( Dest ) )

namespace utils
{
  inline void* Copy(midx aSize, void* SourceInit, void* DestInit)
  {
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    u32 i = 0;
    while (aSize--) { *Dest++ = *Source++;}

    return(DestInit);
  }
  
  // djb2 from http://www.cse.yorku.ca/~oz/hash.html
  inline u32 djb2_hash(const char* str)
  {
    u32 hash = 5381;
    u32 c;

    while (c = *str++)
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
  }
 
}