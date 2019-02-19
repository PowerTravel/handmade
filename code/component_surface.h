#ifndef COMPONENT_SURFACE_H
#define COMPONENT_SURFACE_H
struct bitmap;

struct material
{
	v4 AmbientColor;
	v4 DiffuseColor;
	v4 SpecularColor;
	r32 Shininess;
	bitmap* DiffuseMap;
};

struct component_surface
{
	material* Material;
};


#endif // COMPONENT_SURFACE_PROPERTY_H