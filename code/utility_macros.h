#ifndef UTILITY_MACROS_H
#define UTILITY_MACROS_H

#include "intrinsics.h"

#define GetAbsoluteMax(a,b) ( Abs(a) > Abs(b) ) ? Abs(a) : Abs(b);
#define GetAbsoluteMin(a,b) ( Abs(a) < Abs(b) ) ? Abs(a) : Abs(b);

#define GetMax(a,b) ( (a) > (b) ) ? (a) : (b);
#define GetMin(a,b) ( (a) < (b) ) ? (a) : (b);

#define OffsetOf(type, Member) (umm) &(((type *)0)->Member)

#endif //UTILITY_MACROS_H