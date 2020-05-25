#include "component_sprite_animation.h"
#include "entity_components.h"
#include "handmade.h"
#include "random.h"

void SpriteAnimationSystemUpdate(world* World)
{
  TIMED_FUNCTION();
  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* Entity = &World->Entities[Index];

    if( Entity->SpriteAnimationComponent )
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

      if(Entity->DynamicsComponent)
      {
        v3 Velocity = Entity->DynamicsComponent->LinearVelocity;
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
      }else{

        local_persist u32 Timer = 0;

        if(Timer%100 == 0)
        { 
          r32 r = GetRandomReal(Timer);
          r32 interval = 1.f/4.f;
          if(r < interval)
          {
            SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("jump");
          }else if(r  >= interval && r < 2*interval){
            SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("fall");
          }else if(r  >= 2*interval && r < 3*interval)
          {
            SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("idle1");
          }else{
            SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("run");
          }

          r = GetRandomReal(Timer);
          if( r < 0.5)
          {
            SpriteAnimation->InvertX = false;
          }else{
            SpriteAnimation->InvertX = true;
          }
        }
        Timer++;

      }

    }
  }
}