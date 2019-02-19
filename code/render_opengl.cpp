#include "render_opengl.h"

void PushRenderGroup( render_push_buffer* PushBuffer, component_render_mesh* Mesh )
{
	/*
	mesh_render_group* NewRenderGroup = (mesh_render_group*) PushStruct( PushBuffer->Arena, mesh_render_group );
	NewRenderGroup->Mesh = Mesh;
	PushBuffer->RenderGroups.Push(NewRenderGroup);
	*/
}

void InitiatePushBuffer(render_push_buffer* PushBuffer, game_offscreen_buffer* aOffscreenBuffer, depth_buffer* DepthBuffer, memory_arena* aArena)
{	
	/*
	*PushBuffer = {};
	PushBuffer->Arena = aArena;
	PushBuffer->DepthBuffer = DepthBuffer;
	PushBuffer->TemporaryMemory = BeginTemporaryMemory(PushBuffer->Arena);
	PushBuffer->OffscreenBuffer = aOffscreenBuffer;
	PushBuffer->RenderGroups = filo_queue<mesh_render_group*>( PushBuffer->Arena );
	PushBuffer->Lights = list<component_light*>(PushBuffer->Arena);
	*/
}

void ClearPushBuffer(render_push_buffer* Buffer)
{
	/*
	Assert(Buffer->Arena);
	Assert(Buffer->OffscreenBuffer);
	EndTemporaryMemory(Buffer->TemporaryMemory);
	*Buffer = {};
	*/
}
