#ifndef STRING_H
#define STRING_H


#define STR_MAX_LINE_LENGTH 512
#define STR_MAX_WORD_LENGTH 64


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
BeginsWith( size_t LookForLength, const char* LookForString, size_t SearchInLength, const char* SearchInString )
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


char*
Contains( size_t LookForLength, char* LookForString, size_t SearchInLength, char* SearchInString )
{
	if(SearchInLength<LookForLength  )
	{
		return 0;
	}

	size_t SearchMargin = SearchInLength - LookForLength;
	size_t ScanIdx = 0;
	while(ScanIdx <= SearchMargin)
	{
		size_t LeftOverLength = SearchInLength - SearchMargin;
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


b32
Equals( char* StringA, char* StringB )
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
	// 		 Resulting in a small rounding error. Can this be fixed?
	return Value / Fact;
}

internal void
CopyStrings(	size_t SourceCount, char* Source,
				size_t DestCount,	 char* Dest )
{
	Assert(SourceCount <= DestCount);
	for( s32 Index = 0; Index < SourceCount; ++Index)
	{
		*Dest++ = *Source++;
	}

	*Dest ='\0';
}

internal void
CatStrings(	const size_t SourceACount, const char* SourceA,
			const size_t SourceBCount, const char* SourceB,
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

	*Dest ='\0';
}

}

#endif