#include "handmade.h"
#include "handmade_tile.h"
#include "utility_macros.h"
#include "entity_components.h"
#include "component_camera.h"

internal em_chunk* PushNewChunk(memory_arena* Arena, component_list* List)
{
  em_chunk* Chunk = PushStruct(Arena, em_chunk);
  Chunk->Used = 0;
  Chunk->Memory = PushArray(Arena, List->ChunkSize, u8);
  Chunk->Next = List->First;
  List->First = Chunk;
  return Chunk;
}

internal inline component_head*
CreateComponent(memory_arena* Arena, component_list* ComponentList)
{
  em_chunk* Chunk = ComponentList->First;
  if (!Chunk)
  {
    Chunk = PushNewChunk(Arena, ComponentList);
  }else if ((ComponentList->ChunkSize - Chunk->Used) < ComponentList->ComponentSize){
    // Should be 0 but does not have to...
    // In the future maybe I want to leave space for byte-alignment
    Assert((ComponentList->ChunkSize - Chunk->Used) == 0);
    Chunk = PushNewChunk(Arena, ComponentList);
  }

  component_head* ComponentHead = (component_head*) (Chunk->Memory + Chunk->Used);
  Chunk->Used += ComponentList->ComponentSize;
  ++ComponentList->Count;
  Assert(Chunk->Used <= ComponentList->ChunkSize);

  return ComponentHead;
}

internal inline b32
IndexOfLeastSignificantSetBit( u32 EntityFlags, u32* Index )
{
  bit_scan_result BitScan = FindLeastSignificantSetBit( EntityFlags );
  *Index = BitScan.Index;
  return BitScan.Found;
}

internal inline u32 IndexOfLeastSignificantSetBit( u32 EntityFlags )
{
  bit_scan_result BitScan = FindLeastSignificantSetBit( EntityFlags );
  Assert(BitScan.Found);
  return BitScan.Index;
}

internal inline entity* GetEntityFromID(entity_manager* EM, u32 EntityID)
{
  em_chunk* EntityChunk = EM->EntityList;
  u32 ChunkStartIdx = EM->EntitiesPerChunk * (EM->EntityChunkCount-1);

  u32 EntityIndex = EntityID-1;

  while(EntityIndex < ChunkStartIdx)
  {
    Assert(EntityChunk->Next);
    Assert(ChunkStartIdx>0);
    ChunkStartIdx-=EM->EntitiesPerChunk;
    EntityChunk = EntityChunk->Next;
  }

  Assert(ChunkStartIdx< EM->EntityCount);

  u32 IDWithinChunk = EntityIndex % EM->EntitiesPerChunk;
  entity* Entity = ( (entity*) EntityChunk->Memory ) + IDWithinChunk;
  Assert(Entity->ID == EntityID);
  return Entity;
}

entity_component_chunk* GetNewComponentChunk(memory_arena* Arena, u32 ComponentFlags)
{
  entity_component_chunk* NewChunk = PushStruct(Arena, entity_component_chunk);
  u32 ComponentCount = GetSetBitCount(ComponentFlags);
  NewChunk->Types = ComponentFlags;
  NewChunk->Components = PushArray(Arena, ComponentCount, component_head*);
  return NewChunk;
}

void PopulateChunkWithComponents(entity_manager* EM, entity* Entity, entity_component_chunk* Chunk)
{
  u32 Index = 0;
  u32 FlagsToAdd = Chunk->Types;
  u32 ComponentIndex;
  while(IndexOfLeastSignificantSetBit(FlagsToAdd, &ComponentIndex))
  {
    Assert(ComponentIndex < IndexOfLeastSignificantSetBit(COMPONENT_FLAG_FINAL));
    component_list* ComponentList = EM->Components + ComponentIndex;
    component_head* Head = CreateComponent(&EM->Arena, ComponentList);
    Head->Entity = Entity;
    Head->Type = ComponentList->Type;
    Chunk->Components[Index++] = Head;
    FlagsToAdd -= ComponentList->Type;
  }

  Chunk->Next = Entity->Components;
  Entity->Components = Chunk;
}

void NewComponents(entity_manager* EM, u32 EntityID, u32 ComponentFlags)
{
  CheckArena(&EM->Arena);
  Assert(EntityID != 0);
  Assert(EntityID <= EM->EntityCount);
  Assert( ! ( ComponentFlags & ( ~(COMPONENT_FLAG_FINAL - 1) ) ) );

  entity* Entity = GetEntityFromID(EM, EntityID);
  Assert(( ComponentFlags & Entity->ComponentFlags) == COMPONENT_FLAG_NONE);
  Entity->ComponentFlags = Entity->ComponentFlags | ComponentFlags;

  entity_component_chunk* Chunk = GetNewComponentChunk(&EM->Arena, ComponentFlags);
  PopulateChunkWithComponents(EM, Entity, Chunk);

}

u32 NewEntity( entity_manager* EM )
{
  CheckArena(&EM->Arena);
  em_chunk* Entities = EM->EntityList;

  u32 EntitySize = sizeof(entity);
  u32 ChunkSize = EM->EntitiesPerChunk * EntitySize;
  midx SpaceLeft = ChunkSize - Entities->Used;
  if (SpaceLeft < EntitySize)
  {
    Assert(SpaceLeft == 0);
    EM->EntityList = PushStruct(&EM->Arena, em_chunk);
    EM->EntityList->Memory = PushArray(&EM->Arena, ChunkSize, u8);
    EM->EntityList->Next = Entities;
    Entities = EM->EntityList;
    ++EM->EntityChunkCount;
  }

  entity* NewEntity = (entity*)(Entities->Memory + Entities->Used);
  Entities->Used += EntitySize;
  NewEntity->ID = ++EM->EntityCount;
  NewEntity->Components = 0;

  return NewEntity->ID;
}

internal inline entity_component_chunk*
GetChunkContainingComponent( entity* Entity, u32 ComponentFlag )
{
  entity_component_chunk* Chunk = Entity->Components;
  while(!(Chunk->Types & ComponentFlag))
  {
    Assert(Chunk->Next);
    Chunk = Chunk->Next;
  }
  return Chunk;
}

u32 GetOrderIndexOfBit(u32 BitSet, u32 Bit)
{
  u32 Index=0;
  Assert(BitSet & Bit);
  u32 BitIndex;
  IndexOfLeastSignificantSetBit(Bit,&BitIndex);
  u32 BitSetIndex;
  while(IndexOfLeastSignificantSetBit(BitSet,&BitSetIndex))
  {
    if(BitIndex == BitSetIndex)
    {
      return Index;
    }
    BitSet-= (1<<BitSetIndex);
    ++Index;
  }
  INVALID_CODE_PATH;
  return 0;
}

internal u8* GetComponent(entity_manager* EM, entity* Entity, u32 ComponentFlag)
{
  u8* Result = 0;
  if( !(Entity->ComponentFlags & ComponentFlag) )
  {
    return Result;
  }

  entity_component_chunk* Chunk = GetChunkContainingComponent( Entity, ComponentFlag );
  u32 Index = GetOrderIndexOfBit(Chunk->Types, ComponentFlag);
  component_head* Head = Chunk->Components[Index];
  Assert(Head->Type == ComponentFlag);

  Result = ((u8*) Head) + sizeof(component_head);
  return Result;
}

u8* GetComponent(entity_manager* EM, u32 EntityID, u32 ComponentFlag)
{
  Assert( GetSetBitCount(ComponentFlag) == 1);
  Assert( ComponentFlag != COMPONENT_FLAG_NONE );
  entity* Entity = GetEntityFromID(EM, EntityID);
  u8* Result = GetComponent(EM, Entity, ComponentFlag);
  return Result;
}

struct component_filter_list_entry
{
  filtered_components Entry;
  component_filter_list_entry* Next;
};

component_list* GetListWithLowestCount(entity_manager* EM, u32 ComponentFlags)
{
  component_list* ListWithLowestCount = 0;
  u32 Index = 0;
  u32 SmallestSize = U32Max;
  u32 FlagsToScan = ComponentFlags;
  while(IndexOfLeastSignificantSetBit(FlagsToScan, &Index))
  {
    component_list* List = EM->Components + Index;
    if(List->Count<SmallestSize)
    {
      SmallestSize = List->Count;
      ListWithLowestCount = List;
    }
    FlagsToScan -= List->Type;
  }
  return ListWithLowestCount;
}

internal inline u32 GetUsedCount(component_list* ComponentList, em_chunk* Chunk)
{
  u32 Result = (u32) (Chunk->Used / ComponentList->ComponentSize);
  return Result;
}

internal inline component_head* GetComponentHead(component_list* ComponentList, em_chunk* Chunk, u32 Index)
{
  midx MemOffset = Index * ComponentList->ComponentSize;
  component_head* Result = (component_head*) (Chunk->Memory + MemOffset);
  return Result;
}

internal inline b32 HasAllComponents(component_head* Component, u32 Flags)
{
  b32 Result = (Component->Entity->ComponentFlags & Flags) == Flags;
  return Result;
}

internal u32 FilterForComponentTypes(entity_manager* EM, component_list* ComponentList, u32 ComponentFlags,
  component_filter_list_entry** ReturnList)
{
  u32 FlagsToAdd = ComponentFlags - (ComponentList->Type & ComponentFlags);

  component_filter_list_entry* ListHead = 0;
  u32 ListEntryCount = 0;
  em_chunk* Chunk = ComponentList->First;
  while(Chunk)
  {
    u32 Count = GetUsedCount(ComponentList,Chunk);
    u32 Index = 0;
    component_head* Head = 0;
    filtered_components ResultEntry = {};
    while(Index < Count)
    {
      component_head* Component = GetComponentHead(ComponentList,Chunk,Index);
      if(HasAllComponents(Component, FlagsToAdd))
      {
        if(!ResultEntry.Heads)
        {
          ResultEntry.Heads = Component;
        }
        ++ResultEntry.Count;
      }else{
        if(ResultEntry.Heads)
        {
          component_filter_list_entry* NewListEntry = PushStruct(&EM->Arena, component_filter_list_entry);
          NewListEntry->Entry = ResultEntry;
          NewListEntry->Next = ListHead;
          ListHead = NewListEntry;
          ++ListEntryCount;
          ResultEntry = {};
        }
      }
      ++Index;
    }
    if(ResultEntry.Heads)
    {
      component_filter_list_entry* NewListEntry = PushStruct(&EM->Arena, component_filter_list_entry);
      NewListEntry->Entry = ResultEntry;
      NewListEntry->Next = ListHead;
      ListHead = NewListEntry;
      ++ListEntryCount;
      ResultEntry = {};
    }
    Chunk = Chunk->Next;
  }

  *ReturnList = ListHead;
  return ListEntryCount;
}

component_result* GetComponentsOfType(entity_manager* EM, u32 ComponentFlags)
{
  if(EM->Arena.TempCount == 0)
  {
    *((int*)0) = 0;
  }
  component_list* SmallestList = GetListWithLowestCount(EM, ComponentFlags);

  component_filter_list_entry* ReturnList;
  u32 ListCount = FilterForComponentTypes(EM, SmallestList, ComponentFlags, &ReturnList);

  component_result* Result = PushStruct(&EM->Arena, component_result);
  Result->EM = EM;
  Result->MainType = SmallestList->Type;
  Result->MainTypeSize = SmallestList->ComponentSize;
  Result->Types = ComponentFlags;
  Result->ArrayCount = ListCount;
  Result->ArrayIndex = 0;
  Result->ComponentIndex = 0;
  Result->FilteredArray = PushArray(&EM->Arena, ListCount, filtered_components);
  filtered_components* FilteredEntry = Result->FilteredArray;
  while (ReturnList)
  {
    *FilteredEntry++ = ReturnList->Entry;
    ReturnList = ReturnList->Next;
  }
  return Result;
};

inline internal b32 IsAtEnd(component_result* ComponentList)
{
  b32 Result = ComponentList->ArrayIndex >= ComponentList->ArrayCount;
  return Result;
}

u8* GetComponent(entity_manager* EM, component_result* ComponentList, u32 ComponentFlag)
{
  u8* Result = 0;
  Assert( GetSetBitCount(ComponentFlag) == 1);
  if(IsAtEnd(ComponentList))
  {
    return Result;
  }

  filtered_components* Array = ComponentList->FilteredArray + ComponentList->ArrayIndex;

  Assert(ComponentList->ComponentIndex < Array->Count);
  component_head* Head = (component_head*) (((u8*) Array->Heads) +  ComponentList->MainTypeSize * ComponentList->ComponentIndex);
  if(Head->Type & ComponentFlag)
  {
    Result = ((u8*)Head) + sizeof(component_head);
  }else if(HasAllComponents(Head,ComponentFlag)){
    Result = GetComponent(EM, Head->Entity, ComponentFlag);
  }

  return Result;
}

b32 Next(entity_manager* EM, component_result* ComponentList)
{
  if(!ComponentList->Begun)
  {
    ComponentList->Begun = true;
    ComponentList->ComponentIndex = 0;
    ComponentList->ArrayIndex = 0;
  }else{
    ComponentList->ComponentIndex++;
    if(ComponentList->ComponentIndex >= ComponentList->FilteredArray[ComponentList->ArrayIndex].Count)
    {
      ComponentList->ComponentIndex = 0;
      ++ComponentList->ArrayIndex;
    }
  }
  return !IsAtEnd(ComponentList);
}

entity_manager* CreateEntityManager( )
{
  entity_manager* Result = BootstrapPushStruct(entity_manager, Arena);

  u32 CameraChunkCount = 10;
  u32 LightChunkCount = 10;
  u32 ControllerChunkCount = 4;
  u32 EntityChunkCount = 128;

  CameraChunkCount = 4;
  LightChunkCount = 4;
  ControllerChunkCount = 4;
  EntityChunkCount = 4;

  Result->ComponentCount = IndexOfLeastSignificantSetBit(COMPONENT_FLAG_FINAL);
  Result->Components = PushArray( &Result->Arena, Result->ComponentCount, component_list);
  Result->Components[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_CAMERA)] = ComponentList(COMPONENT_FLAG_CAMERA, sizeof(component_camera), CameraChunkCount, COMPONENT_FLAG_NONE);
  Result->Components[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_LIGHT)] = ComponentList(COMPONENT_FLAG_LIGHT, sizeof(component_light), LightChunkCount, COMPONENT_FLAG_SPATIAL),
  Result->Components[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_CONTROLLER)] = ComponentList(COMPONENT_FLAG_CONTROLLER, sizeof(component_controller), ControllerChunkCount, COMPONENT_FLAG_NONE);
  Result->Components[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_SPATIAL)] = ComponentList(COMPONENT_FLAG_SPATIAL, sizeof(component_spatial), EntityChunkCount, COMPONENT_FLAG_NONE);
  Result->Components[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_COLLIDER)] = ComponentList(COMPONENT_FLAG_COLLIDER, sizeof(component_collider), EntityChunkCount, COMPONENT_FLAG_SPATIAL);
  Result->Components[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_DYNAMICS)] = ComponentList(COMPONENT_FLAG_DYNAMICS, sizeof(component_dynamics), EntityChunkCount, COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_COLLIDER);
  Result->Components[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_RENDER)] = ComponentList(COMPONENT_FLAG_RENDER, sizeof(component_render), EntityChunkCount, COMPONENT_FLAG_NONE);
  Result->Components[IndexOfLeastSignificantSetBit(COMPONENT_FLAG_SPRITE_ANIMATION)] = ComponentList(COMPONENT_FLAG_SPRITE_ANIMATION, sizeof(component_sprite_animation), EntityChunkCount, COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_RENDER);

  Result->EntityCount = 0;
  Result->EntityChunkCount = 1;
  Result->EntitiesPerChunk = EntityChunkCount;
  Result->EntityList = PushStruct( &Result->Arena, em_chunk);
  Result->EntityList->Used = 0;
  Result->EntityList->Memory = PushArray(&Result->Arena, Result->EntitiesPerChunk * sizeof(entity), u8);
  Result->EntityList->Next = 0;

  return Result;
}
