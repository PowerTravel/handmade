#ifndef UTILITY_MACROS_H
#define UTILITY_MACROS_H

#include "intrinsics.h"

#define GetAbsoluteMax(a,b) ( Abs(a) > Abs(b) ) ? Abs(a) : Abs(b);
#define GetAbsoluteMin(a,b) ( Abs(a) < Abs(b) ) ? Abs(a) : Abs(b);

#endif //UTILITY_MACROS_H