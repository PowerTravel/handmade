#pragma once

#include "intrinsics.h"

// Our re-implementation of msvc's _va_args

#if 0
#ifndef _ADDRESSOF
// Get the address of v
#define _ADDRESSOF(v) (&(v))
#endif
#ifndef _INTSIZEOF
// Rounds to the nearest int
#define _INTSIZEOF(n) ( ( sizeof(n) + sizeof(int)-1) & ~(sizeof(int)-1) )
#endif

#ifndef _crt_va_start
// Initiate the pointer ap to where the arguments of function v start
#define _crt_va_start(ap,v) ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v))
#endif
#ifndef _crt_va_arg
// Advance the pointer ap forward (by type size) but return the original position
#define _crt_va_arg(ap,t) ( *(t *) ((ap +=  _INTSIZEOF(t) ) - _INTSIZEOF(t))
#endif
#ifndef _crt_va_end
// Set ap to null
#define _crt_va_end(ap) ( ap += (va_list)0)
#endif


internal u32
FormatStringsList( u32 DestSize, char* Dest, char* Format, arg_list )
{
  char* At = Format;
  while(At[0])
  va_arg();
}
#endif

#define Print(string, ... ) Platform.DEBUGPrint(string, ## __VA_ARGS__ )

#define GetCantorPair(a,b) ((u32) ( (1/2.f) * ((a) + (b)) * ((a) + (b) + 1) + (b) ))
#define Clamp(Val, Min, Max) ((Val) < (Min)) ? (Min) : (((Val) > (Max)) ? (Max) : (Val))

#define ListInitiate( Sentinel )               \
{                                              \
  (Sentinel)->Previous = (Sentinel);           \
  (Sentinel)->Next = (Sentinel);               \
}

#define ListInsertAfter( Sentinel, Element ) \
{                                            \
  (Element)->Previous = (Sentinel);          \
  (Element)->Next = (Sentinel)->Next;        \
  (Element)->Previous->Next = (Element);     \
  (Element)->Next->Previous = (Element);     \
}

#define ListInsertBefore( Sentinel, Element ) \
{                                             \
  (Element)->Previous = (Sentinel)->Previous; \
  (Element)->Next = (Sentinel);               \
  (Element)->Previous->Next = (Element);      \
  (Element)->Next->Previous = (Element);      \
}

#define ListRemove( Element )                      \
{                                                  \
  (Element)->Previous->Next = (Element)->Next;     \
  (Element)->Next->Previous = (Element)->Previous; \
}

#define Maximum(A, B) (((A) > (B)) ? (A) : (B))
#define Minimum(A, B) (((A) < (B)) ? (A) : (B))

#define EnumToIdx(Enum) ((u32) Enum)

#define GetAbsoluteMax(a,b) ( Abs(a) > Abs(b) ) ? Abs(a) : Abs(b);
#define GetAbsoluteMin(a,b) ( Abs(a) < Abs(b) ) ? Abs(a) : Abs(b);

#define OffsetOf(type, Member) (umm) &(((type *)0)->Member)

#define ToBptr( Pointer ) ( (bptr) Pointer )
#define AdvanceBytePointer(Pointer, ByteCount) ToBptr(Pointer)+ (ByteCount);
#define RetreatBytePointer(Pointer, ByteCount) ToBptr(Pointer) - (ByteCount);

#define CopyArray( Count, Source, Dest ) utils::Copy( (Count)*sizeof( *(Source) ), ( Source ), ( Dest ) )

#define ZeroStruct( Instance ) utils::ZeroSize( sizeof( Instance ), &( Instance ) )
#define ZeroArray( Count, Pointer ) utils::ZeroSize( Count*sizeof( ( Pointer )[0] ), Pointer )

#define BranchlessArithmatic( Condition, ExpressionIfTrue, ExpressionIfFalse )  ((s32)(Condition)) * (ExpressionIfTrue) + ((s32) !(Condition) ) * (ExpressionIfFalse)

#define ToArrayIndex(Row, Col, Stride) (Row)  * (Stride) + (Col)

#define NextElement(Base, Stride, Type) (Type*)(((bptr)Base) + ByteStride);


#define HexToColorV4( Red, Green, Blue )  V4((Red)/(r32) 0xFF,(Green)/(r32) 0xFF,(Blue)/(r32)0xFF,1);
#define HexCodeToColorV4( Code )  V4( \
    (((Code) & 0xFF0000) >> 16)/(r32) 0xFF, \
    (((Code) & 0x00FF00) >>  8)/(r32) 0xFF, \
    (((Code) & 0x0000FF) >>  0)/(r32) 0xFF,1)

namespace utils
{
  inline void
  ZeroSize( memory_index Size, void *Ptr )
  {
    u8 *Byte = (u8*) Ptr;
    while(Size--)
    {
      *Byte++ = 0;
    }
  }

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