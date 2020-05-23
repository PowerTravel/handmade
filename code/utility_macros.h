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
  Element->Previous->Next = Element;     \
  Element->Next = Sentinel->Next;        \
  Element->Next->Previous = Element;     \
}

#define DoubleLinkListInsertBefore( Sentinel, Element ) \
{                                         \
  Element->Previous = Sentinel->Previous; \
  Element->Previous->Next = Element;      \
  Element->Next = Sentinel;               \
  Element->Next->Previous = Element;      \
}

#define Maximum(A, B) ((A > B) ? (A) : (B))
#define Minimum(A, B) ((A < B) ? (A) : (B))

#define GetAbsoluteMax(a,b) ( Abs(a) > Abs(b) ) ? Abs(a) : Abs(b);
#define GetAbsoluteMin(a,b) ( Abs(a) < Abs(b) ) ? Abs(a) : Abs(b);

#define OffsetOf(type, Member) (umm) &(((type *)0)->Member)

#define CopyArray( Count, Source, Dest ) utils::Copy( (Count)*sizeof( *(Source) ), ( Source ), ( Dest ) )
inline void* Copy(midx aSize, void* SourceInit, void* DestInit)
{
  u8 *Source = (u8 *)SourceInit;
  u8 *Dest = (u8 *)DestInit;
  while(aSize--) {*Dest++ = *Source++;}

  return(DestInit);
}