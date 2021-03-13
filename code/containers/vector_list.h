#pragma once

template <typename T>
class vector_list
{
  struct list_entry
  {
    list_entry* Next;
    list_entry* Previous;
  };

  struct vec_entry : public list_entry
  {
    T Data;
  };

  s32 m_maxCount;
  s32 m_count;
  list_entry* m_sentinel;
  vec_entry* m_vec;

  void InsertAfter(list_entry* Entry,  list_entry* New);
  void InsertBefore(list_entry* Entry, list_entry* New);

  void Remove( list_entry* Entry );
  void Remove( vec_entry* Entry ){ Remove(static_cast<list_entry*>(Entry)); }

  vec_entry* FirstEntry();
  vec_entry* LastEntry();
  vec_entry* GetEntryFromData(T const * Data);
  vec_entry* InsertNewData(T const * Data, s32 EntryIndex);
  u32 GetFirstFreeIndex();
  b32 Exists(T const * Data);
  void Pivot(vec_entry* A, vec_entry* B);

  public:
    vector_list(memory_arena* Arena, u32 VectorMaxCount);

    b32 Valid(){   return m_sentinel != 0; };
    u32 Size(){    return m_count; };
    u32 maxSize(){ return m_maxCount; };
    b32 IsEmpty(){ return m_count == 0; };

    T* First();
    T* Last();
    T* Next(T const * Data);
    T* Previous(T* Data);
    void Clear();
    T* GetFromVector(u32 VectorIndex);
    T* GetFromList(u32 ListIndex);
    b32 Exists(u32 VectorIndex);
    T* InsertAfter(T const * Element, T const & NewData, s32 EntryIndex = -1);
    T* InsertBefore(T const * Element, T const & NewData, s32 EntryIndex = -1);
    T* PushBack(T const & Data, s32 EntryIndex = -1);
    T* PushFront(T const & Data, s32 EntryIndex = -1);
    T PopFront();
    T PopBack();
    T Pop(T* Data);
    void Swap(T* DataA, T* DataB);
    u32 GetIndexOfEntry(T const * Data);

    // Sorting
    void MergeSort(memory_arena* Arena, b32 (*DifferenceFun) ( T*,  T*));
};



template<typename  T>
void vector_list<T>::InsertAfter(list_entry* Entry,  list_entry* New)
{
  New->Previous = Entry;
  New->Next = Entry->Next;
  New->Previous->Next = New;
  New->Next->Previous = New;
  m_count++;
}

template<typename T>
void vector_list<T>::InsertBefore(list_entry* Entry, list_entry* New)
{
  New->Previous = Entry->Previous;
  New->Next = Entry;
  New->Previous->Next = New;
  New->Next->Previous = New;
  m_count++;
}

template<typename T>
void vector_list<T>::Remove( list_entry* Entry )
{
  Assert(m_count > 0);
  m_count--;
  Entry->Previous->Next = Entry->Next;
  Entry->Next->Previous = Entry->Previous;
  *Entry = {};
}

template<typename T>
typename vector_list<T>::vec_entry* vector_list<T>::FirstEntry()
{
  vec_entry* VecEntry = static_cast<vec_entry*>(m_sentinel->Next);
  return VecEntry;
}

template<typename T>
typename vector_list<T>::vec_entry* vector_list<T>::LastEntry()
{
  vec_entry* VecEntry = static_cast<vec_entry*>(m_sentinel->Previous);
  return VecEntry;
}

template<typename T>
typename vector_list<T>::vec_entry* vector_list<T>::GetEntryFromData(T const * Data)
{
  Assert( (bptr)(Data) > (bptr)(m_vec));
  Assert( (bptr)(Data) < (bptr)((m_vec + m_maxCount)));

  bptr EntryPtr = RetreatBytePointer(Data, OffsetOf(vec_entry, Data));
  Assert( (  (((midx)EntryPtr) - ((midx)m_vec)) % sizeof(vec_entry) == 0) );

  vec_entry* VecEntry =  (vec_entry*)EntryPtr;
  return VecEntry;
}

template<typename T>
typename vector_list<T>::vec_entry* vector_list<T>::InsertNewData(T const * Data, s32 EntryIndex)
{
  vec_entry* NewDataEntry = m_vec + EntryIndex;
  utils::Copy(sizeof(T), (void*) Data, (void*) &NewDataEntry->Data);
  return NewDataEntry;
}

template<typename T>
u32 vector_list<T>::GetFirstFreeIndex()
{
  Assert(m_count < m_maxCount);
  u32 Result = 0;
  while (Exists(Result))
  {
    ++Result;
  }
  return Result;
}


template<typename T>
b32 vector_list<T>::Exists(T const * Data)
{
  u32 Index = GetIndexOfEntry(Data);
  b32 Result = Exists(Index);
  return Result;
}


template<typename T>
void vector_list<T>::Pivot(vec_entry* A, vec_entry* B)
{
  Assert(A->Next == B);
  Assert(B->Previous == A);
  Assert(A != B);
  Assert(A != m_sentinel);
  Assert(B != m_sentinel);

  list_entry* First = A->Previous;
  list_entry* Last = B->Next;

  Last->Previous = A;
  Last->Previous->Next = Last;
  A->Previous = B;

  First->Next = B;
  First->Next->Previous = First;
  B->Next = A;
}

template<typename T>
vector_list<T>::vector_list(memory_arena* Arena, u32 VectorMaxCount) : m_maxCount( VectorMaxCount), m_count(0)
{
  m_sentinel = PushStruct(Arena, list_entry);
  m_vec = PushArray(Arena, VectorMaxCount, vec_entry);
  m_sentinel->Next = m_sentinel;
  m_sentinel->Previous = m_sentinel;
}

template<typename T>
T* vector_list<T>:: First()
{
  T* Result = 0;
  if(m_count > 0)
  {
    Result = &(FirstEntry()->Data);
  }

  return Result;
}

template<typename T>
T* vector_list<T>::Last()
{
  T* Result = 0;
  if(m_count > 0)
  {
    Result = &(LastEntry()->Data);
  }
  return Result;
}

template<typename T>
T* vector_list<T>::Next(T const * Data)
{
  T* Result = 0;

  vec_entry* ListEntry = GetEntryFromData(Data);

  if(ListEntry->Next != m_sentinel)
  {
    vec_entry* VecEntry = static_cast<vec_entry*>(ListEntry->Next);
    Result = &VecEntry->Data;
  }
  return Result;
}

template<typename T>
T* vector_list<T>::Previous(T* Data)
{
  T* Result = 0;
  vec_entry* VecEntry = GetEntryFromData(Data);

  if(VecEntry->Previous != m_sentinel)
  {
    Result = &VecEntry->Data;
  }
  return Result;
}

template<typename T>
void vector_list<T>::Clear()
{
  TIMED_FUNCTION();
  while(m_sentinel->Next != m_sentinel)
  {
    list_entry* EntryToRemove = m_sentinel->Next;
    Remove(EntryToRemove);
  }
  Assert(m_sentinel->Next == m_sentinel);
  Assert(m_sentinel->Previous == m_sentinel);
  m_count = 0;
}

template<typename T>
T* vector_list<T>::GetFromVector(u32 VectorIndex)
{
  Assert( static_cast<s32>(VectorIndex) < m_maxCount);
  T* Result = &m_vec[VectorIndex].Data;
  return Result;
}

template<typename T>
T* vector_list<T>::GetFromList(u32 ListIndex)
{
  list_entry* ListEntry = 0;
  if(static_cast<s32>(ListIndex) < m_count/2)
  {
    ListEntry = m_sentinel->Next;
    s32 Pos = 0;
    while (Pos++ < static_cast<s32>(ListIndex))
    {
      ListEntry = ListEntry->Next;
    }
  }else{
    ListEntry = m_sentinel->Previous;
    s32 Pos = 0;
    s32 Count = m_count - ListIndex - 1;
    while (Pos++ > Count)
    {
      ListEntry = ListEntry->Previous;
    }
  }

  Assert(ListEntry != m_sentinel);
  vec_entry* VecEntry = static_cast<vec_entry*>(ListEntry);
  T* Result = &VecEntry->Data;
  return Result;
}

template<typename T>
b32 vector_list<T>::Exists(u32 VectorIndex)
{
  b32 Result = false;
  vec_entry* Entry = m_vec+VectorIndex;
  if (Entry->Next)
  {
    Assert(Entry->Previous);
    Result = true;
  }
  return Result;
}

template<typename T>
T* vector_list<T>::InsertAfter(T const * Element, T const & NewData, s32 EntryIndex)
{
  Assert(Exists(Element));
  if(EntryIndex != -1)
  {
    EntryIndex = GetFirstFreeIndex();
  }
  Assert(!Exists(EntryIndex));

  vec_entry* NewDataEntry = InsertNewData(&NewData, EntryIndex);
  vec_entry* ExistingElement = GetEntryFromData(Element);

  InsertAfter(ExistingElement, NewDataEntry);
  return &NewDataEntry->Data;
}

template<typename T>
T* vector_list<T>::InsertBefore(T const * Element, T const & NewData, s32 EntryIndex)
{
  Assert(Exists(Element));
  if(EntryIndex != -1)
  {
    EntryIndex = GetFirstFreeIndex();
  }

  Assert(!Exists(EntryIndex));
  vec_entry* NewDataEntry = InsertNewData(&NewData, EntryIndex);
  vec_entry* ExistingElement = GetEntryFromData(Element);

  InsertBefore(ExistingElement, NewDataEntry);
  return &NewDataEntry->Data;
}


template<typename T>
T* vector_list<T>::PushBack(T const & Data, s32 EntryIndex)
{
  if(EntryIndex < 0)
  {
    EntryIndex = GetFirstFreeIndex();
  }

  Assert(EntryIndex < m_maxCount);
  Assert(!Exists(EntryIndex));

  vec_entry* Entry = InsertNewData(&Data, EntryIndex);
  InsertBefore(m_sentinel, Entry);

  return &Entry->Data;
}

template<typename T>
T* vector_list<T>::PushFront(T const & Data, s32 EntryIndex)
{
  if(EntryIndex != -1)
  {
    EntryIndex = GetFirstFreeIndex();
  }
  Assert(EntryIndex < m_maxCount);
  Assert(!Exists(EntryIndex));
  vec_entry* Entry = InsertNewData(&Data, EntryIndex);
  InsertAfter(m_sentinel, Entry);

  return &Entry->Data;
}

template<typename T>
T vector_list<T>::PopFront()
{
  Assert(m_count);
  vec_entry* First = FirstEntry();
  T* Result = &First->Data;
  Remove(First);
  return *Result;
}

template<typename T>
T vector_list<T>::PopBack()
{
  Assert(m_count);
  vec_entry* Last = LastEntry();
  T* Result = &Last->Data;
  Remove(Last);
  return *Result;
}

template<typename T>
T vector_list<T>::Pop(T* Data)
{
  vec_entry* Entry = GetEntryFromData(Data);
  Remove(Entry);
  return *Data;
}

template<typename T>
void vector_list<T>::Swap(T* DataA, T* DataB)
{
  vec_entry* A = GetEntryFromData(DataA);
  vec_entry* B = GetEntryFromData(DataB);

  if (A == B)
  {
    return;
  }
  else if (A->Next == B)
  {
    Pivot(A, B);
  }else if (B->Next == A)
  {
    Pivot(B, A);
  }else{

    list_entry* EntryA = static_cast<list_entry*>(GetEntryFromData(DataA));
    list_entry* EntryB = static_cast<list_entry*>(GetEntryFromData(DataB));

    list_entry Tmp = *EntryA;
    *EntryA = *EntryB;
    *EntryB = Tmp;

    EntryA->Previous->Next = EntryA;
    EntryA->Next->Previous = EntryA;

    EntryB->Previous->Next = EntryB;
    EntryB->Next->Previous = EntryB;
  }
}

template<typename T>
u32 vector_list<T>::GetIndexOfEntry(T const * Data)
{
  vec_entry* Entry = GetEntryFromData(Data);
  u32 Result = (u32)(Entry - m_vec);
  return Result;
}

template<typename T>
void vector_list<T>::MergeSort(memory_arena* Arena, b32 (*DifferenceFun) ( T*,  T*))
{
  ScopedMemory ScopedMem = ScopedMemory(Arena);

  u32 ListCount = m_count;

  list_entry** InputArray = PushArray(Arena, ListCount, list_entry*);
  list_entry** TempArray = PushArray(Arena, ListCount, list_entry*);

  u32 i = 0;
  u32 j = 0;
  while(m_sentinel->Next != m_sentinel)
  {
    InputArray[i++] = m_sentinel->Next;
    Remove(m_sentinel->Next);
  }
  Assert(m_sentinel->Next == m_sentinel);
  Assert(m_count == 0);

  u32 TempArrayIndex = 0;
  u32 LowerBoundIndex_1 = 0;
  u32 HigherBoundIndex_1 = 0;
  u32 LowerBoundIndex_2 = 0;
  u32 HigherBoundIndex_2 = 0;

  for(u32 Size = 1; Size < ListCount; Size = Size*2 )
  {
    u32 CycleCount = 0;
    LowerBoundIndex_1 = 0;
    TempArrayIndex = 0;
    while((LowerBoundIndex_1 + Size) < ListCount)
    {
      HigherBoundIndex_1 = LowerBoundIndex_1 + Size - 1;
      LowerBoundIndex_2  = HigherBoundIndex_1 + 1;
      HigherBoundIndex_2 = LowerBoundIndex_2 + Size - 1;
      if(HigherBoundIndex_2 >= ListCount)
      {
        HigherBoundIndex_2 = ListCount-1;
        ++CycleCount;
      }

      /*Merge the two pairs with lower limits LowerBoundIndex_1 and LowerBoundIndex_2*/
      i = LowerBoundIndex_1;
      j = LowerBoundIndex_2;

      while( (i <= HigherBoundIndex_1) && (j <= HigherBoundIndex_2) )
      {
        ++CycleCount;
        T* DataI = &static_cast<vec_entry*>(InputArray[i])->Data;
        T* DataJ = &static_cast<vec_entry*>(InputArray[j])->Data;
        if( DifferenceFun(DataI, DataJ) )
          TempArray[TempArrayIndex++]=InputArray[i++];
        else
          TempArray[TempArrayIndex++]=InputArray[j++];
      }

      while(i <= HigherBoundIndex_1){
        TempArray[TempArrayIndex++]=InputArray[i++];
        ++CycleCount;
      }

      while(j <= HigherBoundIndex_2){
        TempArray[TempArrayIndex++]=InputArray[j++];
        ++CycleCount;
      }

      /**Merging completed**/
      /*Take the next two pairs for merging */
      LowerBoundIndex_1 = HigherBoundIndex_2 + 1;
    }

    /*any pair left */
    for(i = LowerBoundIndex_1; TempArrayIndex < ListCount; i++)
    {
      TempArray[TempArrayIndex++] = InputArray[i];
      CycleCount++;
    }

    CopyArray(ListCount, TempArray, InputArray);
  }

  for(i = 0; i < ListCount; i++)
  {
    InsertAfter(m_sentinel, InputArray[i]);
  }
}