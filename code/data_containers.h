#pragma once

#include "memory.h"

// Todo: Make all lists return Pointers, not copies.
// Todo: Move list to use Iterators instead of first(), Last(), Next() Etc
//       I do NOT like that the list has an internal state

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
  entry* mPosition;

  memory_arena* mArena;

  u32 mSize;

  void InitializeSentinel( entry* Sentinel )
  {
    *Sentinel = {};
    Sentinel->Next     = Sentinel;
    Sentinel->Previous = Sentinel;
  }

  void InsertBetween( entry* A, entry* B, entry* NewEntry )
  {
    NewEntry->Next = B;
    NewEntry->Next->Previous = NewEntry;
    NewEntry->Previous = A;
    NewEntry->Previous->Next = NewEntry;
  }

  void Dissconect( entry* Entry )
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

  inline void
  RealignSentinel( entry* Sentinel )
  {
    // Any function that returns a entry (not entry*) needs to be realigned.
    // The returned copy points to the first and last entry as it should.
    // However, Since the returned entry is a copy it has a different address
    // thus the First and Last Entry Needs to have their pointers reset to
    // the returned copy.
    Sentinel->Next->Previous = Sentinel;
    Sentinel->Previous->Next = Sentinel;
  };

  void Split(entry* InitialSentinel, const u32 InitialCount,
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

  entry Merge( entry* SentinelA,      u32* CountA,
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

  entry MergeSort( entry* Sentinel, u32 Count, s32 (*DifferenceFun) (const T*, const T*)  )
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

  public:
    list() : mSentinel(0), mPosition(0), mArena(0), mSize(0){};

    list(memory_arena* aArena) : mSentinel(0), mPosition(0), mArena(0), mSize(0)
    {
      Initiate(aArena);
    }

    void MergeSort( s32 (*DifferenceFun) (const T*, const T*) )
    {
      if(mSize <= 1)
      {
        return;
      }
      *mSentinel = MergeSort(mSentinel, mSize, DifferenceFun);
      RealignSentinel(mSentinel);
      First();
    }

    void DeleteDuplicates( b32 (*EqualityFun) (const T *, const T *) )
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

    void Initiate( memory_arena* aArena )
    {
      mArena = aArena;
      mSentinel = (entry*) PushStruct(mArena, entry);
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

template <class T>
class fifo_queue
{
  list<T> List;

  public:

    fifo_queue(){};
    fifo_queue( memory_arena* aArena ) : List(aArena){};

    void Initiate( memory_arena* aArena )
    {
      List.Initiate();
    };

    void Push( const T& Entry )
    {
      List.Last();
      List.InsertAfter(Entry);
    };

    T Pop( )
    {
      List.First();
      T Result = List.Get();
      List.Remove();
      return Result;
    };

    b32 IsEmpty(){ return List.IsEmpty(); };
    u32 GetSize(){ return List.GetSize(); };
};

template <class T>
class filo_queue
{
  list<T> List;

  public:

    filo_queue( ) : List(){};
    filo_queue( memory_arena* aArena ) : List(aArena){};
    void Initiate( memory_arena* aArena )
    {
      List.Initiate();
    };

    void Push( T& Entry )
    {
      List.Last();
      List.InsertBefore(Entry);
    };

    T Pop( )
    {
      List.First();
      T Result = List.Get();
      List.Remove();
      return Result;
    };

    u32 GetSize(){ return List.GetSize(); };
    b32 IsEmpty(){ return List.IsEmpty(); };
};

template <class T>
class vector
{
  T* Base;
  u32 MaxCount;

public:
  vector() : Base(0), Arena(0), MaxCount(0){};
  vector(memory_arena* Arena, u32 Size) : Base(0), MaxCount(Size)
  {
    Initiate(Arena,MaxCount);
  };

  void Initiate(memory_arena* Arena, u32 Size)
  {
    Base = (T*) PushArray(Arena, Size, T);
  }

  void Insert(u32 Idx, const T& Value )
  {
    Assert(Idx<MaxCount);

    T* Pos = Base+Idx;
    *Pos = Value;
  }

  void InsertRange(u32 StartIdx, u32 EndIdx, T* Data)
  {
    Assert(EndIdx < MaxCount);
    memory_index DataLen = sizeof(T) * (EndIdx - StartIdx);
    util::Copy( DataLena, Data, Get(StartIdx));
  }

  T* Get(u32 Idx)
  {
    Assert(Idx<MaxCount);
    return Base+Idx;
  }

  T* GetBuffer()
  {
    return Base;
  }

  u32 GetMaxCount()
  {
    return MaxCount;
  }
};

template <class T>
class hash_map
{
  struct pair
  {
    char* Key;
    u32 KeyLength;
    T Val;
  };

  // djb2 from http://www.cse.yorku.ca/~oz/hash.html
  u32 djb2_hash(const char* str)
  {
    u32 hash = 5381;
    u32 c;

    while (c = *str++)
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
  }

  memory_arena* Arena;
  u32 Size;
  vector< list< pair* > > Map;

  public:
    hash_map( ) : Arena(0),  Size(0), Map(){};
    hash_map( memory_arena* aArena, u32 aSize ) : Arena(aArena),  Size(aSize), Map(aArena, aSize){};

    void Insert( const char Key[], const T& Value)
    {
      u32 Hash = djb2_hash(Key);
      list<pair*>* li = Map.Get(Hash % Size);
      if(li->IsEmpty())
      {
        li->Initiate(Arena);
      }

      pair* Item = (pair*) PushStruct(Arena, pair);
      Item->KeyLength = str::StringLength( Key );
      Item->Key = (char*) PushCopy(Arena, Item->KeyLength*sizeof(char), (void*) Key);
      Item->Val = Value;
      li->Last();
      li->InsertAfter(Item);
    }

    // Returns pointer to the object
    T* Get(const char* Key)
    {
      u32 Hash = djb2_hash(Key);

      list<pair*>* li = Map.Get(Hash % Size);

      for(li->First(); !li->IsEnd(); li->Next() )
      {
        pair* p = li->Get();
        if(str::Equals( p->Key, Key ))
        {
          return &p->Val;
        }
      }

      return 0;
    }
};
