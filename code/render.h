#ifndef RENDER_H
#define RENDER_H

#include "entity_components.h"
#include "data_containers.h"
#include "memory.h"

struct render_group
{
	component_mesh* Mesh;
	component_material* Material;	
};

struct depth_buffer
{
	u32 Width;
	u32 Height;
	r32* Buffer;
};

struct render_push_buffer
{
	memory_arena* Arena;
	temporary_memory TemporaryMemory;

	component_camera* Camera;
	game_offscreen_buffer* OffscreenBuffer;
	depth_buffer* DepthBuffer;
	filo_buffer* RenderGroups;
	filo_buffer* Lights;
};

void InitiatePushBuffer(render_push_buffer* PushBuffer, game_offscreen_buffer* aOffscreenBuffer, depth_buffer* DepthBuffer, memory_arena* aArena);
void ClearPushBuffer(render_push_buffer* Buffer);

void PushLight( render_push_buffer* PushBuffer, component_light* Light );
void PushRenderGroup( render_push_buffer* PushBuffer, component_mesh* Mesh, component_material* Material );

#endif // RENDER_HPP
