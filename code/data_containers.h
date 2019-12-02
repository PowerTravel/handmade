#pragma once

#include "memory.h"

template <class T>
class list
{
  struct entry
  {
    T Data;
    entry* Previous;
    entry* Next;
  };

  entry* Sentinel;
  entry* Position;

  memory_arena* Arena;

  u32 Size;

  void InsertBetween( entry* A, entry* B, entry* NewEntry )
  {
    NewEntry->Previous = A;
    NewEntry->Next = B;

    if(A)
    {
      A->Next = NewEntry;
    }

    if(B)
    {
      B->Previous =NewEntry;
    }

    Position = NewEntry;

    ++Size;
  }

  entry* AllocateNewEntry(const T& Data)
  {
    entry* Result = (entry*) PushStruct(Arena, entry);
    Result->Data = Data;
    return Result;
  }

  public:
    list() : Sentinel(0), Position(0), Arena(0), Size(0){};

    list(memory_arena* aArena) : Sentinel(0), Position(0), Arena(0), Size(0)
    {
      Initiate(aArena);
    }

    void Initiate( memory_arena* aArena )
    {
      Arena = aArena;
      Sentinel = (entry*) PushStruct(Arena, entry);
      Sentinel->Previous = Sentinel;
      Sentinel->Next = Sentinel;
      Position = Sentinel;
    }

    u32 GetSize() { return Size; };
    b32 IsEmpty() { return ( Size==0 ); }
    b32 IsEnd()   { return (Position == Sentinel); };


    void First()   { Position = Sentinel->Next;     };
    void Last()    { Position = Sentinel->Previous; };
    void Next()    { if(!IsEnd()) { Position = Position->Next;     } };
    void Previous(){ if(!IsEnd()) { Position = Position->Previous; } };

    void InsertBefore(const T& Data)
    {
      entry* NewEntry = AllocateNewEntry( Data );
      InsertBetween( Position->Previous, Position, NewEntry );
    };

    void InsertAfter(const T& Data)
    {
      entry* NewEntry = AllocateNewEntry( Data );
      InsertBetween( Position, Position->Next, NewEntry );
    }

    void Remove()
    {
      if(IsEnd()){ return; }

      entry* Base = Position->Previous;
      entry* Next = Position->Next;

      Base->Next = Next;
      Next->Previous = Base;

      Position = Base;
      if(Position == Sentinel)
      {
        Position = Sentinel->Next;
      }

      --Size;
    };

    T Get()
    {
      return Position->Data;
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

  void Insert(u32 Idx, T& Value )
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
    hash_map( ) : Map() {};
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
