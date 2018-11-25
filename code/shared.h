#ifndef SHARED_H
#define SHARED_H

#define CopyArray( Count, Source, Dest ) Copy( (Count)*sizeof( *(Source) ), ( Source ), ( Dest ) )

inline void* Copy(memory_index aSize, void *SourceInit, void *DestInit)
{
	char txtBuffer[256];
	_snprintf_s(txtBuffer, sizeof(txtBuffer), "%d\n", (int) aSize );
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    while(aSize--) {*Dest++ = *Source++;}
    
    return(DestInit);
}


#endif