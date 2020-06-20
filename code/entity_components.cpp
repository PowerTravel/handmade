#include "handmade.h"
#include "handmade_tile.h"
#include "utility_macros.h"
#include "entity_components.h"
#include "component_camera.h"

em_chunk* PushNewChunk(entity_manager* EM, component_list* List)
{
  em_chunk* Chunk = PushStruct(&EM->Arena, em_chunk);
  Chunk->Used = 0;
  Chunk->Memory = PushArray(&EM->Arena, List->ChunkSize, u8);
  Chunk->Previous = List->Last;
  List->Last = Chunk;
  return Chunk;
}

internal inline component_base*
PushComponent(entity_manager* EM, u32 ComponentIndex)
{
  component_list* ComponentList = EM->Components + ComponentIndex;

  if (!ComponentList->Last)
  {
    ComponentList->Last = PushNewChunk(EM, ComponentList);
  }

  em_chunk* Chunk = ComponentList->Last;
  midx RemainingBytes = ComponentList->ChunkSize - Chunk->Used;

  if (RemainingBytes < ComponentList->ComponentSize)
  {
    Assert(RemainingBytes == 0);
    ComponentList->Last = PushNewChunk(EM, ComponentList);
    ComponentList->Last->Previous = Chunk;
    Chunk = ComponentList->Last;
  }

  component_base* ComponentBase = (component_base*) (Chunk->Memory + Chunk->Used);
  Chunk->Used += ComponentList->ComponentSize;

  Assert(Chunk->Used <= ComponentList->ChunkSize);

  return ComponentBase;
}

internal inline b32
GetIndexFromFlag( u32 EntityFlags, u32* Index )
{
  bit_scan_result BitScan = FindLeastSignificantSetBit( EntityFlags );
  *Index = BitScan.Index;
  return BitScan.Found;
}

static inline entity* GetEntityFromID(entity_manager* EM, u32 EntityID)
{
  u32 EntityChunks = EntityID/EM->EntitiesPerChunk;
  u32 EntityChunkSkips = EM->EntitycChunkCount - EntityChunks - 1;
  em_chunk* EntityChunk = EM->EntityList;
  for (u32 i = 0; i < EntityChunkSkips; ++i)
  {
    EntityChunk = EntityChunk->Previous;
  }

  u32 IDWithinChunk = EntityID % EM->EntitiesPerChunk;
  entity* Entity = ( (entity*) EntityChunk->Memory ) + IDWithinChunk;
  Assert(Entity->ID == EntityID);
  return Entity;
}

void NewComponents(entity_manager* EM, u32 EntityID, u32 ComponentFlags)
{
  Assert( ComponentFlags );
  Assert( ! ( ComponentFlags & ( ~(COMPONENT_FLAG_FINAL - 1) ) ) );
  Assert(EntityID < EM->EntityCount);

  entity* Entity = GetEntityFromID(EM, EntityID);

  u32 ComponentsToAdd = ComponentFlags;
  u32 ExistingComponents = Entity->ComponentFlags;
  Assert((ComponentsToAdd & ExistingComponents) == COMPONENT_FLAG_NONE);
  Entity->ComponentFlags = ComponentsToAdd | ExistingComponents;

  entity_component_dlist* Sentinel = Entity->ComponentSentinel;

  u32 ComponentIndex;
  while(GetIndexFromFlag(ComponentsToAdd, &ComponentIndex))
  {
    Assert(ComponentIndex < 32);
    u32 ComponentFlag = 1 << ComponentIndex;
    Assert(EM->Components[ComponentIndex].ComponentTypeFlag == ComponentFlag)

    component_base* ComponentBase = PushComponent(EM, ComponentIndex);
    ComponentBase->TypeFlag = ComponentFlag;
    ComponentBase->Entity = Entity;

    entity_component_dlist* Element = PushStruct(&EM->Arena, entity_component_dlist);
    Element->ComponentBase = ComponentBase;

    // We want to insert components in order
    // (why actually?, Just sort them after insertion)
    // Shut up, complexity is fun.
    // You shut up.

    // Step to one before the Flag we want to insert
    while(Sentinel->Next->ComponentBase && (Sentinel->Next->ComponentBase->TypeFlag < ComponentFlag))
    {
      Sentinel = Sentinel->Next;
    }
    DoubleLinkListInsertAfter( Sentinel, Element );
    Sentinel = Element;

    ComponentsToAdd = ComponentsToAdd - ComponentFlag;
  }
}

u32 NewEntity( entity_manager* EM )
{
  em_chunk* Entities = EM->EntityList;

  u32 EntitySize = sizeof(entity);
  u32 ChunkSize = EM->EntitiesPerChunk * EntitySize;
  midx SpaceLeft = ChunkSize - Entities->Used;
  if (SpaceLeft < EntitySize)
  {
    Assert(SpaceLeft == 0);
    EM->EntityList = PushStruct(&EM->Arena, em_chunk);
    EM->EntityList->Memory = PushArray(&EM->Arena, ChunkSize, u8);
    EM->EntityList->Previous = Entities;
    Entities = EM->EntityList;
    ++EM->EntitycChunkCount;
  }

  entity* NewEntity = (entity*)(Entities->Memory + Entities->Used);
  Entities->Used += EntitySize; 
  NewEntity->ID = EM->EntityCount++;
  NewEntity->ComponentSentinel = PushStruct(&EM->Arena, entity_component_dlist);
  DoubleLinkListInitiate(NewEntity->ComponentSentinel);

  return NewEntity->ID;
}

u8* GetComponent(entity_manager* EM, u32 EntityID, u32 ComponentFlag)
{
  Assert( ComponentFlag != COMPONENT_FLAG_NONE );
  entity* Entity = GetEntityFromID(EM,EntityID);
  u8* Result = 0;
  if( (Entity->ComponentFlags & ComponentFlag) != ComponentFlag )
  {
    return Result;
  }

  entity_component_dlist* Component = Entity->ComponentSentinel->Next;
  while(Component->ComponentBase->TypeFlag != ComponentFlag)
  {
    Component = Component->Next;
  }

  Result = ((u8*) Component->ComponentBase ) + sizeof(component_base);
  return Result;
}


