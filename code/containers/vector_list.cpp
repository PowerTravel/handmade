void MergeSort(memory_arena* Arena, vector_list* List, b32 (*DifferenceFun) ( void*,  void*))
{
  ScopedMemory(GlobalGameState->TransientArena);

  u32 ListCount = List->ListCount;
  list_entry** InputArray = PushArray(Arena, ListCount, list_entry*);
  list_entry** TempArray = PushArray(Arena, ListCount, list_entry*);

  list_entry* Entry = List->Sentinel.Next;
  u32 i = 0;
  u32 j = 0;
  while(Entry != &List->Sentinel)
  {
    list_entry* Tmp = Entry;
    Entry = Entry->Next;
    InputArray[i++] = Tmp;
    ListRemove(Tmp);
  }
  Assert(IsEmpty(List));

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
        void* DataI = (void*) (InputArray[i]+1);
        void* DataJ = (void*) (InputArray[j]+1);
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
    ListInsertAfter(&List->Sentinel, InputArray[i]);
  }
}
