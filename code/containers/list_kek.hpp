#pragma once
#include "vector.h"

template <class T>
class iterator
{
  T* Data;
  entry* Previous;
  entry* Next;
};

template <class T>
class list
{
  struct entry
  {
    T Data;
    entry* Previous;
    entry* Next;
  };

  entry* mSentinel;

  memory_arena* mArena;

  u32 mSize;

  inline void InitializeSentinel( entry* Sentinel )
  {
    *Sentinel = {};
    Sentinel->Next     = Sentinel;
    Sentinel->Previous = Sentinel;
  }

  inline void InsertBetween( entry* A, entry* B, entry* NewEntry )
  {
    NewEntry->Next = B;
    NewEntry->Next->Previous = NewEntry;
    NewEntry->Previous = A;
    NewEntry->Previous->Next = NewEntry;
  }

  inline void Remove( entry* Entry )
  {
    Entry->Previous->Next = Entry->Next;
    Entry->Next->Previous = Entry->Previous;
    Entry->Next = 0;
    Entry->Previous = 0;
  }

  entry* AllocateNewEntry(const T& Data)
  {
    entry* Result = (entry*) PushStruct(mArena, entry);
    Result->Data = Data;
    return Result;
  }

  public:
    list() : mSentinel(0), mArena(0), mSize(0){};

    list(memory_arena* aArena) : mSentinel(0), mArena(0), mSize(0)
    {
      Initiate(aArena);
    }

    void Initiate( memory_arena* aArena )
    {
      mArena = aArena;
      mSentinel = (entry*) PushStruct(mArena, entry);
      InitializeSentinel(mSentinel);
    }

    u32 GetSize() { return mSize; };
    b32 IsEmpty() { return ( mSize==0 ); }

    entry* First()
    {
      return mSentinel->Next;
    };

    entry* Last()
    {
      return mSentinel->Previous;
    };

    entry* Next(entry* Entry)
    {
      if(!IsEnd(Entry))
      {
        
      }
      return &mPosition->Data;
    };

    T* Previous()
    {
      if(!IsEnd())
      {
        mPosition = mPosition->Previous;
      }
      return &mPosition->Data;
    };

    T* InsertBefore(const T& Data)
    {
      entry* NewEntry = AllocateNewEntry( Data );
      InsertBetween( mPosition->Previous, mPosition, NewEntry );
      mPosition = NewEntry;
      ++Size;
      return &Position->Data;
    };

    T* InsertAfter(const T& Data)
    {
      entry* NewEntry = AllocateNewEntry( Data );
      InsertBetween( mPosition, mPosition->Next, NewEntry );
      mPosition = NewEntry;
      ++mSize;
      return &mPosition->Data;
    }

    b32 PushBackUnique(const T& Data, b32 (*EqualityFun) (const T *, const T *) )
    {
      entry* Scanner = aSentinel->Next;
      while(Scanner != aSentinel)
      {
        if(EqualityFun(&Data, Scanner))
        {
          return false;
        }
      }

      entry* NewEntry = AllocateNewEntry( Data );
      InsertBetween( mSentinel->Previous, mSentinel, NewEntry );
      ++mSize;
      return true;
    }

    //TODO: This function is unsafe to use in a for-loop. Remove;
    //      The list was written so one could do the follwoing:
    //        while( !List.IsEmpty() ){ List.Remove() }; Which is fine.
    //      And
    //        for( List.First(); !List.IsEmpty(); List.Next() ){ Do Something }; Also fine
    //      However combining them turned out to be dangerous.
    //        for( List.First(); !List.IsEmpty(); List.Next() ){ if(Condition){ List.Remove(); } }; Not fine!
    /// But using 'for' and 'remove' this way can cause the following:
    //      If remove is called on the first entry the second entry becomes the first
    //      The list is then pointing to the new first enty. When it goes to the top of the
    //      for-loop Next is called and then points to the second entry which used to be the third.
    //      The second entry is thus scipped.
    //      However IF remove is NOT called on the first entry all items are looped through and it
    //      works as intended. A thing that works 99 percent of the time is way more dangerous than
    //      something that always fails.
    void Remove()
    {
      if(IsEnd()){ return; }

      entry* Base = mPosition->Previous;
      entry* Next = mPosition->Next;

      Base->Next = Next;
      Next->Previous = Base;

      mPosition = Base;
      if(mPosition == mSentinel)
      {
        mPosition = mSentinel->Next;
      }

      --mSize;
    };

    T Get()
    {
      return mPosition->Data;
    };

    T* GetRef()
    {
      return &mPosition->Data;
    };


    T Get(u32 Idx)
    {
      Assert(Idx < GetSize());
      u32 i = 0;
      for (First(); !IsEnd(); Next())
      {
        if(i==Idx)
        {
          ++i;
          return Get();
        }
      }

      INVALID_CODE_PATH;
      return {};
    };
};
