#ifndef RENDER_H
#define RENDER_H

struct render_state
{
	// textures
	// Lights
	// Render_WireFrame
	// Render_BoundingBox
	// Backface culling on
	// Gourad shading
	// blinn phong shading
};

struct render_group
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
	game_offscreen_buffer* OffscreenBuffer;
	depth_buffer* DepthBuffer;
	filo_queue<render_group*> RenderGroups;
	list<component_light*> Lights;
};

void InitiatePushBuffer(render_push_buffer* PushBuffer, game_offscreen_buffer* aOffscreenBuffer, depth_buffer* DepthBuffer, memory_arena* aArena);
void ClearPushBuffer(render_push_buffer* Buffer);

void PushLight( render_push_buffer* PushBuffer, component_light* Light );
void PushRenderGroup( render_push_buffer* PushBuffer, component_render_mesh* Mesh );

#endif // RENDER_HPP
