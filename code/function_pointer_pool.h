#pragma once

MENU_HANDLE_INPUT(HandleInput);

MENU_UPDATE_CHILD_REGIONS(UpdateChildRegions);
MENU_UPDATE_CHILD_REGIONS(RootUpdateChildRegions);
MENU_UPDATE_CHILD_REGIONS(UpdateSplitChildRegions);
MENU_UPDATE_CHILD_REGIONS(UpdateHBFChildRegions);
MENU_UPDATE_CHILD_REGIONS(UpdateGridChildRegions);
MENU_UPDATE_CHILD_REGIONS(DebugCollationGraphUpdateChildRegions);

MENU_DRAW(Draw);
MENU_DRAW(BorderDraw);
MENU_DRAW(DrawFunctionTimeline);

BUTTON_ATTRIBUTE_UPDATE(TabButtonUpdate);
BUTTON_ATTRIBUTE_UPDATE(DebugToggleButton);
BUTTON_ATTRIBUTE_UPDATE(DebugRecompileButton);
BUTTON_ATTRIBUTE_UPDATE(DebugPauseCollationButton);

DRAGGABLE_ATTRIBUTE_UPDATE(UpdateFrameBorderCallback);
DRAGGABLE_ATTRIBUTE_UPDATE(SplitWindowHeaderDrag);
DRAGGABLE_ATTRIBUTE_UPDATE(UpdateHeaderPosition);
DRAGGABLE_ATTRIBUTE_UPDATE(TabDrag); 
DRAGGABLE_ATTRIBUTE_UPDATE(UpdateSplitBorderCallback);

#ifdef HANDMADE_INTERNAL

#define RedeclareFunction(Name) _DeclareFunction((func_ptr_void) (&Name), #Name );
#define NewFunPtr(FunctionName) if(str::ExactlyEquals(Function->Name, #FunctionName)) { RedeclareFunction(FunctionName); } else

void _ReinitiatePool(function_pool* Pool)
{
  u32 PoolSize = Pool->Count;
  for (u32 Index = 0; Index < PoolSize; ++Index)
  {
    function_ptr* Function = &Pool->Functions[Index];
    NewFunPtr(HandleInput)
    NewFunPtr(UpdateChildRegions)
    NewFunPtr(RootUpdateChildRegions)
    NewFunPtr(UpdateSplitChildRegions)
    NewFunPtr(UpdateHBFChildRegions)
    NewFunPtr(UpdateGridChildRegions)
    NewFunPtr(Draw)
    NewFunPtr(BorderDraw)
    NewFunPtr(TabButtonUpdate)
    NewFunPtr(UpdateHeaderPosition)
    NewFunPtr(TabDrag)
    NewFunPtr(UpdateSplitBorderCallback)
    NewFunPtr(DebugToggleButton)
    NewFunPtr(DebugRecompileButton)
    NewFunPtr(DebugPauseCollationButton)
    NewFunPtr(DebugCollationGraphUpdateChildRegions)
    NewFunPtr(DrawFunctionTimeline)
    NewFunPtr(SplitWindowHeaderDrag)
    NewFunPtr(UpdateFrameBorderCallback)
    {
      INVALID_CODE_PATH;
    }    
  }

}

#define ReinitiatePool() _ReinitiatePool(GlobalGameState->FunctionPool)

#else

#define ReinitiatePool()

#endif
