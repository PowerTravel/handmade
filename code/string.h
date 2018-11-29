#ifndef STRING_H
#define STRING_H

#include "types.h"
#include "shared.h"


namespace str
{

internal s32
StringLength( char* String )
{
	// foo++ -> increment after
	// ++foo -> increment before
	s32 Count = 0;
	while(*String++)
	{
		++Count;
	}
	return Count;
}

internal bool
BeginsWith( size_t LookForLength,  char* LookForString, size_t SearchInLength,  char* SearchInString )
{
	if( LookForLength > SearchInLength )
	{
		return false;
	}

	size_t idx = 0;
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
	// 		 Resulting in a small rounding error. Can this be fixed?
	return Value / Fact;
}

internal void
CatStrings(	size_t SourceACount, char* SourceA,
			size_t SourceBCount, char* SourceB,
			size_t DestCount,	 char* Dest )
{

	for( s32 Index = 0; Index < SourceACount; ++Index)
	{
		*Dest++ = *SourceA++;
	}


	for( s32 Index = 0; Index < SourceBCount; ++Index)
	{
		*Dest++ = *SourceB++;
	}

	*Dest++ ='\0';
}

}

#endif