
void SpatialSystemUpdate( world* World )
{
	v3 Gravity = V3( 0, -9, 0 );
	v3 Collition = V3( 0, 0, 0 );
	r32 dt = 1/60.f;
	for(u32 Index = 0;  Index < World->NrEntities; ++Index )
	{
		entity* E = &World->Entities[Index];

		if( E->Types & COMPONENT_TYPE_SPATIAL )
		{
			component_spatial* S = E->SpatialComponent;
			S->Position = S->Position + (dt/2) * S->Velocity;

			if(S->IsDynamic)
			{
				#if 0
				S1->Velocity += t * Gravity;
				S1->Position += t * S1->Velocity;

				// TODO (Jakob): Make spatial data system. This is slow.
				for( u32 EntityIndexTwo = 0; EntityIndexTwo < World->NrEntities; ++EntityIndexTwo )
				{
					entity* E2 = &World->Entities[EntityIndexTwo];
					if( ( E2->Types & COMPONENT_TYPE_SPATIAL ) && (E2->id != E1->id) )
					{
						component_spatial* S2 = E2->SpatialComponent;
						r32 S2Top = S2->Position.Y + (S2->AABBDimensions.Y/2.f);
						r32 S2Bot = S2->Position.Y - (S2->AABBDimensions.Y/2.f);
						
						r32 S1Top = S1->Position.Y + (S1->AABBDimensions.Y/2.f);
						r32 S1Bot = S1->Position.Y - (S1->AABBDimensions.Y/2.f);
						if( S1Bot < S2Top )
						{
							S1->Position.Y = S2Top + (S1->AABBDimensions.Y/2.f);
							S1->Velocity -= S1->Velocity*1.2;
						}
					}
				}
				
				#else
				
				//E1->SpatialComponent->RotationAngle += 0.03;
				//E1->SpatialComponent->RotationAxis = V3(0,0,1);

				#endif 
			}
		}
	}
}
