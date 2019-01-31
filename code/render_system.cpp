#include "entity_components.h"

void RenderSystemUpdate( world* World, render_push_buffer* RenderPushBuffer )
{
	for( u32 Index = 0;  Index < World->NrEntities; ++Index )
	{
		entity* Entity = &World->Entities[Index];

		if( ( Entity->Types & COMPONENT_TYPE_CAMERA ) )
		{
			RenderPushBuffer->Camera = Entity->CameraComponent;
		}

		if( ( Entity->Types & COMPONENT_TYPE_LIGHT ) )
		{
			PushLight( RenderPushBuffer, Entity->LightComponent );
		}

		if( ( Entity->Types & ( COMPONENT_TYPE_RENDER_MESH  | COMPONENT_TYPE_SPATIAL ) ) )
		{
			Entity->RenderMeshComponent->T = M4Identity();
			Translate( V4( Entity->SpatialComponent->Position, 1), Entity->RenderMeshComponent->T);
		}

		if( ( Entity->Types & ( COMPONENT_TYPE_RENDER_MESH  ) )  )
		{
			PushRenderGroup( RenderPushBuffer, Entity->RenderMeshComponent );
		}

	}
}