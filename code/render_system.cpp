#include "entity_components.h"

void RenderSystemUpdate( world* World, render_push_buffer* RenderPushBuffer )
{
	for( s32 Index = 0;  Index < ArrayCount( World->WorldEntities); ++Index )
	{
		entity* Entity = &World->WorldEntities[Index];

		if( ( Entity->Types & COMPONENT_TYPE_CAMERA ) )
		{
			game_offscreen_buffer* Buffer = RenderPushBuffer->OffscreenBuffer;
			camera_component& c = Entity->CameraComponent; 
			RenderPushBuffer->R = M4( c.ScreenWidth/2.f,                  0, 0,  c.ScreenWidth/2.f, 
				    			                      0, c.ScreenHeight/2.f, 0, c.ScreenHeight/2.f, 
				    			                      0,                  0, 1,                  0,
				    			                      0,                  0, 0,                  1);
			RenderPushBuffer->V = c.V;
			RenderPushBuffer->P = c.P;
		}

		if( ( Entity->Types & COMPONENT_TYPE_MESH ) )
		{
			obj_geometry* O = Entity->GeometryComponent.Object;

			m4 T = Entity->GeometryComponent.T;
			m4 ModelView = RenderPushBuffer->V * T;
			for( s32 i = 0; i < O->nt; ++i)
			{
				triangle& t = O->t[i];

				render_entity Triangle = {};

				Triangle.normal = T*t.n;
				if(Triangle.normal * V4(0,1,0,0) > 0 )
				{
					Triangle.AmbientProduct  	= V4(0.0,0.7,0,0);
					Triangle.DiffuseProduct  	= V4(0,0.0,0,1);
					Triangle.SpecularProduct 	= V4(0,0,0.0,1);
				}else{
					Triangle.AmbientProduct  	= V4(0.4,0,0,1);
					Triangle.DiffuseProduct  	= V4(0,0.0,0,1);
					Triangle.SpecularProduct 	= V4(0,0,0.0,1);
				}
				Triangle.LightPosition 		= V4(1,1,1,1);
			
				Triangle.vertices[0] = T*O->v[ t.vi[0] ];
				Triangle.vertices[1] = T*O->v[ t.vi[1] ];
				Triangle.vertices[2] = T*O->v[ t.vi[2] ];


				Triangle.verticeNormals[0] = T*O->vn[ t.vni[0] ];
				Triangle.verticeNormals[1] = T*O->vn[ t.vni[1] ];
				Triangle.verticeNormals[2] = T*O->vn[ t.vni[2] ];

				PushBuffer( RenderPushBuffer, Triangle );
			}
		}
	}
}