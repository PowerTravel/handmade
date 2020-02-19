#pragma once

#include "intrinsics.h"

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

#define GetAbsoluteMax(a,b) ( Abs(a) > Abs(b) ) ? Abs(a) : Abs(b);
#define GetAbsoluteMin(a,b) ( Abs(a) < Abs(b) ) ? Abs(a) : Abs(b);

#define GetMax(a,b) ( (a) > (b) ) ? (a) : (b);
#define GetMin(a,b) ( (a) < (b) ) ? (a) : (b);

#define OffsetOf(type, Member) (umm) &(((type *)0)->Member)
