#include "list.hpp"

void list::InsertBetween( entry* A, entry* B, entry* NewEntry )
{
  NewEntry->Next = B;
  NewEntry->Next->Previous = NewEntry;
  NewEntry->Previous = A;
  NewEntry->Previous->Next = NewEntry;
}

void list::Dissconect( entry* Entry )
{
  Entry->Previous->Next = Entry->Next;
  Entry->Next->Previous = Entry->Previous;
  Entry->Next = 0;
  Entry->Previous = 0;
}

inline void
list::RealignSentinel( entry* Sentinel )
{
  Sentinel->Next->Previous = Sentinel;
  Sentinel->Previous->Next = Sentinel;
};

void list::Split(entry* InitialSentinel, const u32 InitialCount,
             entry* SentinelA, u32* CountA,
             entry* SentinelB, u32* CountB)
{
  InitializeSentinel(SentinelA);
  InitializeSentinel(SentinelB);

  *CountA = Roof(InitialCount/2);
  *CountB = InitialCount - *CountA;

  if((*CountA==0) && (*CountB==0))
  {
    return;
  }

  entry* Pos = InitialSentinel;
  for(u32 i = 0; i < *CountA; ++i)
  {
    Pos = Pos->Next;
  }

  entry* ALast  = Pos;
  entry* BFirst = Pos->Next;


  SentinelA->Next = InitialSentinel->Next;
  SentinelA->Next->Previous = SentinelA;
  SentinelA->Previous = ALast;
  SentinelA->Previous->Next = SentinelA;

  if(*CountB!=0)
  {
    SentinelB->Next = BFirst;
    SentinelB->Next->Previous = SentinelB;
    SentinelB->Previous = InitialSentinel->Previous;
    SentinelB->Previous->Next = SentinelB;
  }
}

list::entry list::Merge( entry* SentinelA,      u32* CountA,
             entry* SentinelB,      u32* CountB,
             s32 (*DifferenceFun) (const T*, const T*) )
{
  entry* PositionA = SentinelA->Next;
  entry* PositionB = SentinelB->Next;

  entry ResultSentinel = {};
  InitializeSentinel(&ResultSentinel);
  entry* Position = &ResultSentinel;

  u32 TotCount = *CountA+*CountB;
  u32 Count = 0;
  while(Count < TotCount)
  {
    if( (PositionA != SentinelA) &&
        (PositionB != SentinelB))
    {
      entry* EntryToInsert = 0;
      s32 DiffVal = DifferenceFun(&PositionA->Data, &PositionB->Data);
      if(DiffVal >= 0)
      {
        EntryToInsert = PositionA;
        PositionA = PositionA->Next;
      }else{
        EntryToInsert = PositionB;
        PositionB = PositionB->Next;
      }

      Dissconect(EntryToInsert);
      InsertBetween( Position, Position->Next, EntryToInsert);
      Position = EntryToInsert;
      ++Count;
    }else{
      if( (PositionA != SentinelA) )
      {
        ResultSentinel.Previous->Next = SentinelA->Next;
        SentinelA->Next->Previous = ResultSentinel.Previous;

        SentinelA->Previous->Next = &ResultSentinel;
        ResultSentinel.Previous  = SentinelA->Previous;

        Count = TotCount;
        *SentinelA = {};
      }else{
        ResultSentinel.Previous->Next = SentinelB->Next;
        SentinelB->Next->Previous = ResultSentinel.Previous;

        SentinelB->Previous->Next = &ResultSentinel;
        ResultSentinel.Previous  = SentinelB->Previous;

        Count = TotCount;
        *SentinelB = {};
      }
    }
  }

  return ResultSentinel;
}

list::entry list::MergeSort( entry* Sentinel, u32 Count, s32 (*DifferenceFun) (const T*, const T*)  )
{
  entry SentinelA, SentinelB;
  u32 CountA ,CountB;

  Split(  Sentinel,   Count,
         &SentinelA, &CountA,
         &SentinelB, &CountB);

  entry ResultA = {};
  if(CountA > 1)
  {
    ResultA = MergeSort(&SentinelA, CountA, DifferenceFun);
  }else{
    ResultA = SentinelA;
  }

  RealignSentinel(&ResultA);

  entry ResultB = {};
  if(CountB > 1)
  {
    ResultB = MergeSort(&SentinelB, CountB, DifferenceFun);
  }else{
    ResultB = SentinelB;
  }
  RealignSentinel(&ResultB);

  entry Result = Merge( &ResultA, &CountA,
                        &ResultB, &CountB,
                        DifferenceFun);
  RealignSentinel(&Result);

  return Result;
}

list::list(memory_arena* aArena) : mSentinel(0), mPosition(0), mArena(0), mSize(0)
{
  Initiate(aArena);
}

void MergeSort( list List, s32 (*DifferenceFun) (const T*, const T*) )
{
  if(mSize <= 1)
  {
    return;
  }
  *mSentinel = MergeSort(List, mSentinel, mSize, DifferenceFun);
  RealignSentinel(mSentinel);
}

void list::DeleteDuplicates( b32 (*EqualityFun) (const T *, const T *) )
{
  entry* ScannerA = First();
  while(ScannerA != aSentinel)
  {
    entry* ScannerB = ScannerA->Next;
    while(ScannerB != aSentinel)
    {
      if(EqualityFun(&ScannerA->Data, &ScannerB->Data))
      {
        entry* Duplicate = ScannerB;
        ScannerB = ScannerB->Previous;
        Dissconect( Duplicate );
      }

      ScannerB = ScannerB->Next;
    }
    ScannerA = ScannerA->Next;
  }
}

void list::Initiate(   )
{
  InitializeSentinel(mSentinel);
  mPosition = mSentinel;
}

    u32 GetSize() { return mSize; };
    b32 IsEmpty() { return ( mSize==0 ); }
    b32 IsEnd()   { return (mPosition == mSentinel); };

    T* First()
    {
      mPosition = mSentinel->Next;
      return &mPosition->Data;
    };

    T* Last()
    {
      mPosition = mSentinel->Previous;
      return &mPosition->Data;
    };

    T* Next()
    {
      if(!IsEnd())
      {
        mPosition = mPosition->Next;
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
