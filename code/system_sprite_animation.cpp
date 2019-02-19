#include "component_sprite_animation.h"


void SpriteAnimationSystemUpdate(world* World)
{
	for(u32 Index = 0;  Index < World->NrEntities; ++Index )
	{
		entity* Entity = &World->Entities[Index];

		if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
		{
			component_sprite_animation* SAC = Entity->SpriteAnimationComponent;
			Assert(  SAC->ActiveSeries );
			Assert(  SAC->ActiveSeries->ActiveFrame );

			sprite_series* ActiveSeries = SAC->ActiveSeries;
			v3& Velocity = Entity->SpatialComponent->Velocity;
			sprite_type Type = SPRITE_TYPE_NA;
			if( Norm( Velocity ) != 0  )
			{
				Type = SPRITE_TYPE_WALK;
			}else{
				Type = SPRITE_TYPE_IDLE;
			}

			sprite_orientation Orientation = SPRITE_ORIENTATION_M0P;
			r32 VelocityX = Velocity * V3(1,0,0);
			r32 VelocityY = Velocity * V3(0,1,0);
			
			if( VelocityY <= 0 )
			{
				if( VelocityX < 0 )
				{
					Orientation = SPRITE_ORIENTATION_M0P;
				}else{
					Orientation = SPRITE_ORIENTATION_P0P;
				}
			}else{
				if(VelocityX <= 0)
				{
					Orientation = SPRITE_ORIENTATION_M0M;
				}else{
					Orientation = SPRITE_ORIENTATION_P0M;
				}
			}

			SAC->ActiveSeries = GetSeriesWithTypeAndOrientation(SAC, Type, Orientation );
			Assert(SAC->ActiveSeries);
			local_persist u32 FrameCounter = 0;
			if( FrameCounter++ == 7)
			{
				FrameCounter = 0;
				ActiveSeries->ActiveFrame++;
				umm FrameIdx = ActiveSeries->ActiveFrame - ActiveSeries->Frames;
				if(FrameIdx >= ActiveSeries->NrFrames)
				{
					ActiveSeries->ActiveFrame = ActiveSeries->Frames;
				}
			}

		}
	}	
}