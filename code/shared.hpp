

#define CopyArray(Count, Source, Dest) Copy((Count)*sizeof(*(Source)), (Source), (Dest))

inline void* Copy(memory_index aSize, void *SourceInit, void *DestInit)
{
	char txtBuffer[256];
	_snprintf_s(txtBuffer, sizeof(txtBuffer), "%d\n", (int) aSize );
    uint8 *Source = (uint8 *)SourceInit;
    uint8 *Dest = (uint8 *)DestInit;
    while(aSize--) {*Dest++ = *Source++;}
    
    return(DestInit);
}
