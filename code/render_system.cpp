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

		if( ( Entity->Types & ( COMPONENT_TYPE_RENDER_MESH  ) )  )
		{
			PushRenderGroup( RenderPushBuffer, Entity->RenderMeshComponent );
		}

	}
}