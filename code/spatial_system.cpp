
void SpatialSystemUpdate( world* World )
{
	v3 Gravity = V3( 0, -9, 0 );
	v3 Collition = V3( 0, 0, 0 );
	r32 t = 0.01;
	for(u32 Index = 0;  Index < World->NrEntities; ++Index )
	{
		entity* E1 = &World->Entities[Index];

		if( E1->Types & COMPONENT_TYPE_SPATIAL )
		{
			component_spatial* S1 = E1->SpatialComponent;

			if(S1->IsDynamic)
			{
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
			}
		}
	}
}