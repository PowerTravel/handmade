
void SpriteAnimationSystemUpdate(world* World)
{
  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* Entity = &World->Entities[Index];

    if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
    {
//      Assert(Entity->Types & COMPONENT_TYPE_DYNAMICS);

      component_sprite_animation* SpriteAnimation = Entity->SpriteAnimationComponent;
      Assert(  SpriteAnimation->ActiveSeries );

      list<m4>* ActiveSeries = SpriteAnimation->ActiveSeries;
      local_persist u32 FrameCounter = 0;
      if( FrameCounter++ == 10)
      {
        FrameCounter = 0;
        ActiveSeries->Next();
        if(ActiveSeries->IsEnd())
        {
          ActiveSeries->First();
        }
      }
    }
  }
}