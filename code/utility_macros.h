#pragma once

#include "intrinsics.h"

#define DoubleLinkListInsertAfter( Sentinel, Element ) \
(                                        \
  Element->Previous = Sentinel;          \
  Element->Next = Sentinel->Next;        \
  Element->Previous->Next = Element;     \
  Element->Next->Previous = Element;     \
)

#define DoubleLinkListInsertBefore( Sentinel, Element ) \
(                                         \
  Element->Previous = Sentinel->Previous; \
  Element->Next = Sentinel;               \
  Element->Previous->Next = Element;      \
  Element->Next->Previous = Element;      \
)

#define GetAbsoluteMax(a,b) ( Abs(a) > Abs(b) ) ? Abs(a) : Abs(b);
#define GetAbsoluteMin(a,b) ( Abs(a) < Abs(b) ) ? Abs(a) : Abs(b);

#define GetMax(a,b) ( (a) > (b) ) ? (a) : (b);
#define GetMin(a,b) ( (a) < (b) ) ? (a) : (b);

#define OffsetOf(type, Member) (umm) &(((type *)0)->Member)
