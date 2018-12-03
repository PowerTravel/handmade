#ifndef DATA_CONTAINER_H
#define DATA_CONTAINER_H

#include "memory.h"

struct filo_buffer_entry
{
	void* Data;
	filo_buffer_entry* Next;
};

struct filo_buffer
{
	memory_arena* Arena;
	u32 Size;
	filo_buffer_entry* First;
};

template <typename T>
void Push( filo_buffer* FiloBuffer, T* FiloEntry )
{
	filo_buffer_entry* NewEntry = (filo_buffer_entry*) PushStruct(FiloBuffer->Arena, filo_buffer_entry );
	NewEntry->Data = (void*) FiloEntry;
	NewEntry->Next = FiloBuffer->First;
	FiloBuffer->First = NewEntry;
	++FiloBuffer->Size;
};

template <typename T>
T* Pop( filo_buffer* FiloBuffer )
{
	if(!FiloBuffer->First)
	{
		return 0;
	}

	T* Result = (T*) FiloBuffer->First->Data;
	FiloBuffer->First = FiloBuffer->First->Next;
	--FiloBuffer->Size;
	return Result;
};


filo_buffer* CreateFiloBuffer( memory_arena* Arena )
{
	filo_buffer* Result = (filo_buffer*) PushStruct( Arena, filo_buffer );
	Result->Arena = Arena;
	return Result;
};

#endif