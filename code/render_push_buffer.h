#ifndef RENDER_PUSH_BUFFER_H
#define RENDER_PUSH_BUFFER_H

#include "entity_components.h"
#include "component_camera.h"
#include "component_surface.h"

enum render_type
{
	RENDER_TYPE_ENTITY,
	RENDER_TYPE_SPRITE,
	RENDER_TYPE_FLOOR_TILE,
	RENDER_TYPE_WIREBOX
};

struct entry_type_entity
{
	entity* Entity;
};

struct entry_type_sprite
{
	m4 M;
	bitmap* Bitmap;
	rect* Coordinates;
};


typedef entry_type_sprite entry_type_floor_tile;


struct entry_type_wirebox
{
	r32 x;
	r32 y;
	r32 w;
	r32 h;
};

struct push_buffer_header
{
	u32 Type;
	u32 SortKey;
	push_buffer_header* Next;
};

struct render_push_buffer
{
	m4 ProjectionMatrix;
	m4 ViewMatrix;
	game_assets* Assets;
	push_buffer_header* First;
	game_render_commands* RenderCommands;
};

#endif // RENDER_PUSH_BUFFER_H
