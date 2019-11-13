#ifndef RENDER_PUSH_BUFFER_H
#define RENDER_PUSH_BUFFER_H

#include "entity_components.h"
#include "component_camera.h"
#include "component_surface.h"
#include "component_light.h"

enum class render_type
{
	LIGHT,
	MESH,
	SPRITE,
	FLOOR_TILE,
	WIREBOX
};

struct entry_type_light
{
	v4 Color;
	m4 M;
};

struct entry_type_mesh
{
	component_mesh* Mesh;
	component_surface* Surface;
	m4 M;
	m4 NM;
};

struct entry_type_sprite
{
	m4 M;
	bitmap* Bitmap;
	rect2f* Coordinates;
};

typedef entry_type_sprite entry_type_floor_tile;

struct entry_type_wirebox
{
	rect2f Rect;
};

struct push_buffer_header
{
	render_type Type;
	u32 SortKey;
	push_buffer_header* Next;
};

struct render_push_buffer
{
	m4 ProjectionMatrix;
	m4 ViewMatrix;
	game_assets* Assets;
	push_buffer_header* First;
};

#endif // RENDER_PUSH_BUFFER_H
