#ifndef RENDER_OPENGL_H
#define RENDER_OPENGL_H

#include "memory.h"
#include "entity_components.h"
#include "data_containers.h"

struct mesh_render_group
{
	component_render_mesh* Mesh;
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
	filo_queue<entity*> RenderEntities;
};

void InitiatePushBuffer(render_push_buffer* RenderPushBuffer, memory_arena* Arena);
void ClearPushBuffer(render_push_buffer* RenderPushBuffer);

#endif // RENDER_OPENGL_H
