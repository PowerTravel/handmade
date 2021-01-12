#pragma once

#define STR_MAX_LINE_LENGTH 512
#define STR_MAX_WORD_LENGTH 64

#include "math/vector_math.h"
#include "types.h"
#include "intrinsics.h"

namespace str
{

internal u32
StringLength( const char* String )
{
  if(!String){return 0;}

  u32 Count = 0;
  while(*String++)
  {
    ++Count;
  }
  return Count;
}

internal bool
BeginsWith( memory_index LookForLength, const char* LookForString, memory_index SearchInLength, const char* SearchInString )
{
  if( LookForLength > SearchInLength )
  {
    return false;
  }

  memory_index idx = 0;
  while(idx < LookForLength)
  {
    if(SearchInString[idx] != LookForString[idx])
    {
      return false;
    }
    ++idx;
  }

  return true;
}


char*
Contains( memory_index LookForLength, char* LookForString, memory_index SearchInLength, char* SearchInString )
{
  if(SearchInLength<LookForLength  )
  {
    return 0;
  }

  memory_index SearchMargin = SearchInLength - LookForLength;
  memory_index ScanIdx = 0;
  while(ScanIdx <= SearchMargin)
  {
    memory_index LeftOverLength = SearchInLength - SearchMargin;
    char* LeftOverString =  &SearchInString[ScanIdx];
    if( ( *LeftOverString == *LookForString ) &&
      BeginsWith( LookForLength, LookForString, LeftOverLength, LeftOverString) )
    {
      return LeftOverString;
    }

    ++ScanIdx;
  }

  return 0;
}

char*
Contains( char* LookForString, char* SearchInString )
{
  return Contains( StringLength(LookForString), LookForString, StringLength(SearchInString), SearchInString );
}


b32 Equals( const char* StringA, const char* StringB )
{
  while( *StringA && *StringB )
  {
    if( *StringA++ != *StringB++ )
    {
      return false;
    }
  }

  return true;
}

s32 Compare( const char* StringA, const char* StringB )
{
  while( *StringA && *StringB )
  {
    s32 A = (s32)(*StringA++);
    s32 B = (s32)(*StringB++);
    if( A > B )
    {
      return 1;
    }else if(B > A){
      return -1;
    }
  }

  return 0;
}

b32 ExactlyEquals( const char* StringA, const char* StringB )
{
  b32 Result = Equals(StringA, StringB)  && (str::StringLength(StringA) == str::StringLength(StringB));
  return Result;
}
internal char*
FindFirstOf( char* Tokens, char* String )
{
  if( !String ){ return 0; }

  while( *String )
  {
    char* t = Tokens;
    while( *t )
    {
      if( *String == *t )
      {
        return String;
      }
      ++t;
    }
    ++String;
  }

  return 0;
}

internal char*
FindFirstOf( char Token, char* String )
{
  char tokens[2] = {Token, '\0'};
  return FindFirstOf( tokens, String );
}

internal char*
FindFirstNotOf( char* Tokens, char* String )
{
  if( !String ){ return 0; }

  while( *String )
  {
    char* t = Tokens;
    bool found = false;
    while( *t )
    {
      if(*t == *String)
      {
        found = true;
        break;
      }
      ++t;
    }
    if(!found)
    {
      return String;
    }
    ++String;
  }

  return 0;
}

internal char*
FindFirstNotOf( char Token, char* String )
{
  char tokens[2] = {Token, '\0'};
  return FindFirstNotOf( tokens, String );
}

internal char*
FindLastOf( char* Tokens, char* String )
{
  if( !String ){ return 0; }


  u32 Length = StringLength( String );
  if(Length < 1){ return 0; }

  char* ScanPos = String + Length;
  while( ScanPos > String  )
  {
    --ScanPos;

    char* t = Tokens;
    bool found = false;
    while( *t )
    {
      if(*t == *ScanPos)
      {
        found = true;
        break;
      }
      ++t;
    }
    if(found)
    {
      return ScanPos;
    }
  }

  return 0;
}

internal char*
FindLastNotOf( char* Tokens, char* String )
{
  if( !String ){ return 0; }


  u32 Length = StringLength( String );
  if(Length < 1){ return 0; }

  char* ScanPos = String + Length;
  while( ScanPos > String  )
  {
    --ScanPos;

    char* t = Tokens;
    bool found = false;
    while( *t )
    {
      if(*t == *ScanPos)
      {
        found = true;
        break;
      }
      ++t;
    }
    if(!found)
    {
      return ScanPos;
    }
  }

  return 0;
}

internal s32
GetWordCount( char* String )
{
  char* wcp = str::FindFirstNotOf(" \t\n", String);
  s32 wc = wcp ? 1 : 0;
  while(wcp)
  {
    wcp = str::FindFirstOf(" \t", wcp);
    wcp = str::FindFirstNotOf(" \t", wcp);
    wc += wcp ? 1 : 0;
  }

  return wc;
}

r64 StringToReal64( char* String )
{
  char* wcp = String;
  r64 Value = 0;
  r64 Fact = 1;
  b32 PointSeen = false;
  while(*wcp)
  {
    if( *wcp == '-' )
    {
      Fact = -1;
    } else if( *wcp == '.' )
    {
      PointSeen = true;
    }else{
      s32 Digit = *wcp - '0';
      if( (Digit >= 0) || (Digit <=9) )
      {
        if( PointSeen ){
          Fact *= 10;
        }

        Value = 10*Value + Digit;

      } else{
        Assert(0);
      }
    }

    ++wcp;
  }

  // Todo: Divinging by a power of 10 like this is not possible in binary
  //     Resulting in a small rounding error. Can this be fixed?
  return Value / Fact;
}

void CopyStrings(  memory_index SourceCount, char* Source,
        memory_index DestCount,  char* Dest )
{
  Assert(SourceCount <= DestCount);
  for( s32 Index = 0; Index < SourceCount; ++Index)
  {
    *Dest++ = *Source++;
  }

  *Dest ='\0';
}
void CopyStringsUnchecked( const c8* Source, c8* Dest )
{
  while(*Source!= '\0')
  {
    *Dest++ = *Source++;
  }
  *Dest = '\0';
}

void CatStrings( const memory_index SourceACount, const char* SourceA,
      const memory_index SourceBCount, const char* SourceB,
      memory_index DestCount,  char* Dest )
{
  for( s32 Index = 0; Index < SourceACount; ++Index)
  {
    *Dest++ = *SourceA++;
  }


  for( s32 Index = 0; Index < SourceBCount; ++Index)
  {
    *Dest++ = *SourceB++;
  }

  *Dest ='\0';
}


// C program for implementation of ftoa()  from
// https://www.geeksforgeeks.org/convert-floating-point-number-string/

// Reverses a string 'str' of length 'len'
internal void
reverse(char* str, s32 len)
{
  s32 i = 0;
  s32 j = len - 1;
  char temp;
  while (i < j)
  {
    temp   = str[i];
    str[i] = str[j];
    str[j] = temp;
    i++;
    j--;
  }
}

// Converts a given integer x to string str[].
// d is the number of digits required in the output.
// If d is more than the number of digits in x,
// then 0s are added at the beginning.
internal s32
intToStr(s32 x, char str[], s32 d)
{
   Assert(x>=0);
  s32 i = 0;
  while(x)
  {
    str[i++] = (x % 10) + '0';
    x = x / 10;
  }

  // If number of digits required is more, then
  // add 0s at the beginning
  while(i < d)
  {
    str[i++] = '0';
  }

  reverse(str, i);
  str[i] = '\0';
  return i;
}

memory_index itoa(s32 Integer, memory_index StringCount, char* DestString)
{
  memory_index Pos = 0;
  char* Scanner = DestString;
  if(Integer < 0)
  {
    *Scanner++ = '-';
    Integer = -Integer;
    Pos = 1;
  }

  Scanner += intToStr(Integer, Scanner, 1);
  Assert(Scanner < (DestString + StringCount) );
  return Scanner - DestString;
}

// Converts a floating-point/double number to a string.
memory_index ftoa(r32 RealNumber, s32 DecimalPrecision, memory_index StringCount, char* DestString)
{
  char* Scanner = DestString;
  if(RealNumber < 0)
  {
    *Scanner++ = '-';
    RealNumber = -RealNumber;
  }

  // Extract integer part
  s32 ipart = (s32)RealNumber;

  // Extract floating part
  r32 fpart = RealNumber - (r32)ipart;

  // convert integer part to string
  Scanner += intToStr(ipart, Scanner, 1);
  // check for display option after point
  if(DecimalPrecision != 0)
  {
    *Scanner++ = '.'; // add dot

    // Get the value of fraction part upto given no.
    // of points after dot. The third parameter
    // is needed to handle cases like 233.007
    fpart = Round( fpart * Pow(10.f,  (r32) DecimalPrecision));

    Scanner += intToStr((s32)fpart, Scanner, DecimalPrecision);
  }

  Assert(Scanner < (DestString + StringCount) );
  return Scanner - DestString;
}


memory_index ToString( const v3 Vector, const u32 DecimalPrecision, const memory_index DestCount, char* const DestString )
{
  char* Scanner = DestString;
  for (int i = 0; i < 3; ++i)
  {
    memory_index Len = ftoa( Vector.E[i], DecimalPrecision, DestCount, Scanner);
    Scanner += Len;
    Assert( Scanner < (DestCount + DestString - 1) );
    *Scanner++ = ' ';
  }
  *(--Scanner) = '\0';
  return (Scanner - DestString);
}

} // Namespace str