#pragma once

#define DLL_Initiate( Sentinel )          \
{                                          \
  Sentinel->Previous = Sentinel;           \
  Sentinel->Next = Sentinel;               \
}

#define DLL_InsertAfter( Sentinel, Element ) \
{                                              \
  Element->Previous = Sentinel;                \
  Element->Next = Sentinel->Next;              \
  Element->Previous->Next = Element;           \
  Element->Next->Previous = Element;           \
}

#define DLL_InsertBefore( Sentinel, Element ) \
{                                               \
  Element->Previous = Sentinel->Previous;       \
  Element->Next = Sentinel;                     \
  Element->Previous->Next = Element;            \
  Element->Next->Previous = Element;            \
}
