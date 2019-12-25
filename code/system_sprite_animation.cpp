
void SpriteAnimationSystemUpdate(world* World)
{
  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* Entity = &World->Entities[Index];

    if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
    {
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

      if(Entity->Types & COMPONENT_TYPE_DYNAMICS)
      {
        v3 Velocity = Entity->DynamicsComponent->Velocity;
        if(Velocity.Y > 0)
        {
          SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("jump");
        }else if(Velocity.Y <0){
          SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("fall");
        }else if( Abs(Velocity.X) <=0.1)
        {
          SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("idle1");
        }else{
          SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("run");
        }

        if( Velocity.X > 0.1)
        {
          SpriteAnimation->InvertX = false;
        }else if(Velocity.X < -0.1)
        {
          SpriteAnimation->InvertX = true;
        }
      }

    }
  }
}