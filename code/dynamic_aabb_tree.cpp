#include "dynamic_aabb_tree.h"
#include "entity_components.h"
#include "memory.h"

inline internal bool
IsLeaf( aabb_tree_node* Node )
{
  return Node->EntityID!=0;
}

inline internal memory_index
Size(aabb_tree_node** Base,
     aabb_tree_node** Head)
{
  Assert(Head >= Base);
  return (Head - Base);
}

inline internal aabb_tree_node**
Push(aabb_tree_node** Head,
     aabb_tree_node*  Node)
{
  ++Head;
  *Head = Node;
  return Head;
}

inline internal aabb_tree_node**
Pop(aabb_tree_node** Head)
{
  --Head;
  return Head;
}

inline internal aabb_tree_node**
Push2(aabb_tree_node** Head,
      aabb_tree_node*  Node)
{
  Head+=2;
  *Head = Node;
  return Head;
}

inline internal aabb_tree_node**
Pop2(aabb_tree_node** Head)
{
  Head-=2;
  return Head;
}

inline internal aabb_tree_node*
Get(aabb_tree_node** Head)
{
  return *Head;
}

u32 GetAABBList(aabb_tree* Tree, aabb3f** Result)
{
  aabb_tree_node** const Base = PushArray(GlobalGameState->TransientArena, Tree->Size, aabb_tree_node*);
  aabb_tree_node** Head = Base;

  *Result = PushArray(GlobalGameState->TransientArena, Tree->Size, aabb3f);
  aabb3f* AABBList = *Result;

  u32 Count = 0;

  // Init the stacks
  aabb_tree_node* CurrentNode = Tree->Root;
  while( CurrentNode || (Size(Base,Head) > 0) )
  {
    while(CurrentNode)
    {
      Head = Push(Head, CurrentNode);
      CurrentNode = CurrentNode->Left;
    }
    CurrentNode = Get(Head);
    Head  = Pop(Head);

    aabb_tree_node* ParentNode = Get(Head);

    *AABBList++ = CurrentNode->AABB;
    Assert(Count < Tree->Size);
    Count++;

    CurrentNode = CurrentNode->Right;
  }

  return Tree->Size;
}

broad_phase_result_stack* GetCollisionPairs( aabb_tree* Tree, u32* ResultStackSize)
{
  TIMED_FUNCTION();
  *ResultStackSize = 0;
  if(Tree->Size < 2)
  {
    return 0;
  }
  aabb_tree_node** const Base = (aabb_tree_node**) PushArray(GlobalGameState->TransientArena, 2*Tree->Size, aabb_tree_node*);
  aabb_tree_node** const LeftBase  = Base;
  aabb_tree_node** const RightBase = Base+1;
  broad_phase_result_stack* ResultHead = {};
  aabb_tree_node** LeftHead  = LeftBase;
  aabb_tree_node** RightHead = RightBase;

  // Init the stacks
  LeftHead  = Push2(LeftHead,  Tree->Root->Left);
  RightHead = Push2(RightHead, Tree->Root->Right);
  while( Size(LeftBase,LeftHead) > 0 )
  {
    aabb_tree_node* Left  = Get(LeftHead);
    aabb_tree_node* Right = Get(RightHead);

    if( IsLeaf(Left) )
    {
      if( IsLeaf(Right) )
      {
        // Both are leaves
        if(AABBIntersects(&Left->AABB, &Right->AABB))
        {
          broad_phase_result_stack* NewResult = (broad_phase_result_stack*) PushStruct(GlobalGameState->TransientArena ,broad_phase_result_stack);
          NewResult->EntityIDA = Left->EntityID;
          NewResult->EntityIDB = Right->EntityID;
          NewResult->Previous = ResultHead;
          ResultHead = NewResult;
          ++(*ResultStackSize);
        }
        // Pop the leafs
        LeftHead  = Pop2(LeftHead);
        RightHead = Pop2(RightHead);
      }else{
        // Left is leaf, Right is Node
        if(!Right->Entered)
        {
          Right->Entered = true;
          LeftHead =  Push2(LeftHead,  Right->Left);
          RightHead = Push2(RightHead, Right->Right);
        }else{
          LeftHead =  Pop2(LeftHead);
          RightHead = Pop2(RightHead);

          if(AABBIntersects(&Left->AABB, &Right->AABB))
          {
            LeftHead  = Push2(LeftHead,  Left);
            RightHead = Push2(RightHead, Right->Left);
            LeftHead  = Push2(LeftHead,  Left);
            RightHead = Push2(RightHead, Right->Right);
          }
        }
      }
    }else{
      if(IsLeaf(Right))
      {
        // Left is Node, Right is leaf
        if(!Left->Entered)
        {
          Left->Entered = true;
          LeftHead  = Push2(LeftHead,  Left->Left);
          RightHead = Push2(RightHead, Left->Right);
        }else{
          LeftHead =  Pop2(LeftHead);
          RightHead = Pop2(RightHead);
          if(AABBIntersects(&Left->AABB, &Right->AABB))
          {
            LeftHead  = Push2(LeftHead,  Left->Left);
            RightHead = Push2(RightHead, Right);
            LeftHead  = Push2(LeftHead,  Left->Right);
            RightHead = Push2(RightHead, Right);
          }
        }
      }else{
        // Left is Node, Right is Node
        if( !Left->Entered )
        {
          Left->Entered = true;
          LeftHead  = Push2(LeftHead,  Right->Left);
          RightHead = Push2(RightHead, Right->Right);
        }else if( !Right->Entered ){
          Right->Entered = true;
          LeftHead  = Push2(LeftHead,  Left->Left);
          RightHead = Push2(RightHead, Left->Right);
        }else{
          // Pop2 the top head and add all the cross terms
          // This ensures that we wont end up in a crossterm-infinity-loop
          LeftHead  = Pop2(LeftHead);
          RightHead = Pop2(RightHead);
          if(AABBIntersects(&Left->AABB, &Right->AABB))
          {
            LeftHead  = Push2(LeftHead,  Left->Left);
            RightHead = Push2(RightHead, Right->Left);
            LeftHead  = Push2(LeftHead,  Left->Left);
            RightHead = Push2(RightHead, Right->Right);

            LeftHead  = Push2(LeftHead,  Left->Right);
            RightHead = Push2(RightHead, Right->Left);
            LeftHead  = Push2(LeftHead,  Left->Right);
            RightHead = Push2(RightHead, Right->Right);
          }
        }
      }
    }
  }
  return ResultHead;
}

internal aabb_tree_node**
GetLeastVolumeincreaseBranch(aabb_tree_node* Parent,  aabb3f* LeafAABB)
{
  r32 PreLeftVolume   = GetSize(&Parent->Left->AABB);
  r32 PreRightVolume  = GetSize(&Parent->Right->AABB);
  aabb3f LeftMerge    = MergeAABB(Parent->Left->AABB,  *LeafAABB);
  aabb3f RightMerge   = MergeAABB(Parent->Right->AABB, *LeafAABB);
  r32 PostLeftVolume  = GetSize(&LeftMerge);
  r32 PostRightVolume = GetSize(&RightMerge);
  r32 LeftDiff = PostLeftVolume - PreLeftVolume;
  r32 RightDiff = PostRightVolume - PreRightVolume;

  aabb_tree_node** Result = 0;
  if(LeftDiff <= RightDiff)
  {
    Result = &Parent->Left;
  }else{
    Result = &Parent->Right;
  }

  return Result;
}

void AABBTreeInsert( memory_arena* Arena, aabb_tree* Tree, u32 EntityID , aabb3f& AABBWorldSpace )
{
  TIMED_FUNCTION();
  aabb_tree_node* Leaf = (aabb_tree_node*) PushStruct(Arena, aabb_tree_node);
  Leaf->EntityID = EntityID;
  Leaf->AABB   = AABBWorldSpace;

  if(Tree->Size == 0)
  {
    Tree->Root = Leaf;
    Tree->Size = 1;
    return;
  }

  aabb_tree_node** CurrentNodePtr = &Tree->Root;
  aabb_tree_node*  CurrentNode    = *CurrentNodePtr;
  while(!IsLeaf(CurrentNode))
  {
    CurrentNode->AABB = MergeAABB(CurrentNode->AABB, Leaf->AABB);
    CurrentNodePtr    = GetLeastVolumeincreaseBranch(CurrentNode, &Leaf->AABB);
    CurrentNode       = *CurrentNodePtr;
  }

  // We are at leaf
  aabb_tree_node* Node = (aabb_tree_node*) PushStruct(Arena, aabb_tree_node);
  Node->Left  = Leaf;
  Node->Right = CurrentNode;
  Node->AABB  = MergeAABB(Node->Left->AABB, Node->Right->AABB);
  *CurrentNodePtr = Node;
  Tree->Size+=2;
}